// Source Code <Display Commander> // follow this order for includes in all files + add this comment at the top
#include "presentmon_minimal_etw.hpp"
#include "../../utils/logging.hpp"
#include "../../utils/srwlock_wrapper.hpp"

// Libraries <standard C++>
#include <atomic>
#include <cstddef>
#include <cstring>
#include <cwchar>
#include <exception>
#include <thread>

// Libraries <Windows.h>
#include <Windows.h>

// Libraries <Windows>
#include <evntrace.h>
#include <evntcons.h>

namespace display_commander::features::presentmon {
namespace {

struct TraceProperties : EVENT_TRACE_PROPERTIES {
    wchar_t logger_name[128];
};

constexpr GUID kWin32kProviderGuid = {0x8C416C79, 0xD49B, 0x4F01, {0xA4, 0x67, 0xE5, 0x6D, 0x3A, 0xA8, 0x23, 0x4C}};
constexpr uint16_t kWin32kTokenCompositionSurfaceObjectInfoId = 0x00c9;
constexpr uint16_t kWin32kTokenStateChangedInfoId = 0x012d;
constexpr uint64_t kLiveThresholdMs = 3000;

struct RuntimeState {
    SRWLOCK lock = SRWLOCK_INIT;
    PresentMonMode mode = PresentMonMode::Unknown;
    ULONGLONG last_update_ms = 0;
    bool has_data = false;
    bool session_failed = false;
    bool session_running = false;
    DWORD target_pid = 0;
};

RuntimeState g_state{};
std::atomic<bool> g_thread_started{false};
std::atomic<TRACEHANDLE> g_session_handle{0};
std::atomic<TRACEHANDLE> g_trace_handle{INVALID_PROCESSTRACE_HANDLE};
std::thread g_trace_thread;
wchar_t g_session_name[128] = {};

void UpdateMode(PresentMonMode mode) {
    utils::SRWLockExclusive lock(g_state.lock);
    g_state.mode = mode;
    g_state.last_update_ms = GetTickCount64();
    g_state.has_data = true;
}

void CALLBACK EtwRecordCallback(EVENT_RECORD* record) {
    if (record == nullptr) {
        return;
    }
    const auto& hdr = record->EventHeader;
    if (hdr.ProviderId.Data1 != kWin32kProviderGuid.Data1 || hdr.ProviderId.Data2 != kWin32kProviderGuid.Data2
        || hdr.ProviderId.Data3 != kWin32kProviderGuid.Data3) {
        return;
    }
    if (std::memcmp(hdr.ProviderId.Data4, kWin32kProviderGuid.Data4, sizeof(hdr.ProviderId.Data4)) != 0) {
        return;
    }
    if (hdr.ProcessId != g_state.target_pid) {
        return;
    }

    if (hdr.EventDescriptor.Id == kWin32kTokenCompositionSurfaceObjectInfoId) {
        UpdateMode(PresentMonMode::ComposedFlip);
        return;
    }

    if (hdr.EventDescriptor.Id == kWin32kTokenStateChangedInfoId) {
        // Keep current classification but refresh freshness for current-process token state activity.
        utils::SRWLockExclusive lock(g_state.lock);
        if (g_state.has_data) {
            g_state.last_update_ms = GetTickCount64();
        }
    }
}

ULONG CALLBACK EtwBufferCallback(EVENT_TRACE_LOGFILEW* /*unused*/) { return TRUE; }

void StopTraceSessionBestEffort() {
    const TRACEHANDLE trace_handle = g_trace_handle.exchange(INVALID_PROCESSTRACE_HANDLE);
    if (trace_handle != INVALID_PROCESSTRACE_HANDLE) {
        CloseTrace(trace_handle);
    }

    const TRACEHANDLE session_handle = g_session_handle.exchange(0);
    if (session_handle != 0) {
        EnableTraceEx2(session_handle, &kWin32kProviderGuid, EVENT_CONTROL_CODE_DISABLE_PROVIDER, TRACE_LEVEL_NONE, 0, 0,
                       0, nullptr);

        TraceProperties session_props = {};
        session_props.Wnode.BufferSize = static_cast<ULONG>(sizeof(session_props));
        session_props.LoggerNameOffset = offsetof(TraceProperties, logger_name);
        ControlTraceW(session_handle, g_session_name, &session_props, EVENT_TRACE_CONTROL_STOP);
    }

    utils::SRWLockExclusive lock(g_state.lock);
    g_state.session_running = false;
}

void TraceThreadMain() {
    const DWORD current_pid = GetCurrentProcessId();
    {
        utils::SRWLockExclusive lock(g_state.lock);
        g_state.target_pid = current_pid;
        g_state.session_failed = false;
        g_state.session_running = false;
    }

    TraceProperties session_props = {};
    session_props.Wnode.BufferSize = static_cast<ULONG>(sizeof(session_props));
    session_props.Wnode.Flags = WNODE_FLAG_TRACED_GUID;
    session_props.Wnode.ClientContext = 1;  // QPC
    session_props.LogFileMode = EVENT_TRACE_REAL_TIME_MODE;
    session_props.LoggerNameOffset = offsetof(TraceProperties, logger_name);
    session_props.BufferSize = 64;
    session_props.MinimumBuffers = 8;
    session_props.MaximumBuffers = 64;
    std::wcsncpy(session_props.logger_name, g_session_name, _countof(session_props.logger_name) - 1);

    TRACEHANDLE session_handle = 0;
    ULONG start_status = StartTraceW(&session_handle, g_session_name, &session_props);
    if (start_status != ERROR_SUCCESS) {
        utils::SRWLockExclusive lock(g_state.lock);
        g_state.session_failed = true;
        g_state.session_running = false;
        return;
    }
    g_session_handle.store(session_handle);

    ULONG enable_status =
        EnableTraceEx2(session_handle, &kWin32kProviderGuid, EVENT_CONTROL_CODE_ENABLE_PROVIDER, TRACE_LEVEL_INFORMATION, 0,
                       0, 0, nullptr);
    if (enable_status != ERROR_SUCCESS) {
        LogInfo("PresentMon ETW: failed to enable Win32k provider (status=%lu)", enable_status);
        {
            utils::SRWLockExclusive lock(g_state.lock);
            g_state.session_failed = true;
            g_state.session_running = false;
        }
        StopTraceSessionBestEffort();
        return;
    }

    EVENT_TRACE_LOGFILEW trace_log = {};
    trace_log.LoggerName = g_session_name;
    trace_log.ProcessTraceMode = PROCESS_TRACE_MODE_REAL_TIME | PROCESS_TRACE_MODE_EVENT_RECORD;
    trace_log.EventRecordCallback = EtwRecordCallback;
    trace_log.BufferCallback = EtwBufferCallback;
    trace_log.Context = nullptr;

    TRACEHANDLE trace_handle = OpenTraceW(&trace_log);
    if (trace_handle == INVALID_PROCESSTRACE_HANDLE) {
        {
            utils::SRWLockExclusive lock(g_state.lock);
            g_state.session_failed = true;
            g_state.session_running = false;
        }
        StopTraceSessionBestEffort();
        return;
    }
    g_trace_handle.store(trace_handle);
    {
        utils::SRWLockExclusive lock(g_state.lock);
        g_state.session_running = true;
    }

    ProcessTrace(&trace_handle, 1, nullptr, nullptr);
    StopTraceSessionBestEffort();
}

}  // namespace

void EnsurePresentMonEtwStarted() {
    if (g_thread_started.load(std::memory_order_acquire)) {
        return;
    }

    bool expected = false;
    if (!g_thread_started.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
        return;
    }

    const DWORD pid = GetCurrentProcessId();
    std::swprintf(g_session_name, _countof(g_session_name), L"DisplayCommander_PresentMon_%lu", pid);
    try {
        g_trace_thread = std::thread(&TraceThreadMain);
    } catch (const std::exception& e) {
        LogInfo("PresentMon ETW: failed to start trace thread (%s)", e.what());
        {
            utils::SRWLockExclusive lock(g_state.lock);
            g_state.session_failed = true;
            g_state.session_running = false;
        }
        g_thread_started.store(false, std::memory_order_release);
    }
}

void ShutdownPresentMonEtw() {
    StopTraceSessionBestEffort();
    if (g_trace_thread.joinable()) {
        g_trace_thread.join();
    }
    g_thread_started.store(false, std::memory_order_release);
}

PresentMonStateSnapshot GetPresentMonStateSnapshot() {
    PresentMonStateSnapshot snapshot{};
    const ULONGLONG now_ms = GetTickCount64();
    utils::SRWLockShared lock(g_state.lock);
    snapshot.mode = g_state.mode;
    snapshot.has_data = g_state.has_data;
    snapshot.session_running = g_state.session_running;
    snapshot.session_failed = g_state.session_failed;
    if (g_state.has_data && now_ms >= g_state.last_update_ms) {
        snapshot.age_ms = static_cast<uint64_t>(now_ms - g_state.last_update_ms);
    } else {
        snapshot.age_ms = 0;
    }
    snapshot.is_live = snapshot.has_data && snapshot.age_ms <= kLiveThresholdMs;
    return snapshot;
}

const char* PresentMonModeToString(PresentMonMode mode) {
    switch (mode) {
        case PresentMonMode::ComposedFlip: return "Composed: Flip";
        case PresentMonMode::Unknown:
        default: return "Unknown";
    }
}

}  // namespace display_commander::features::presentmon

