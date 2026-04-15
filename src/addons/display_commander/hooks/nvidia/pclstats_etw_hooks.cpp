// Source Code <Display Commander> // follow this order for includes in all files + add this comment at the top
#include "pclstats_etw_hooks.hpp"
#include "../../globals.hpp"
#include "../../utils/general_utils.hpp"
#include "../../utils/logging.hpp"
#include "../../utils/string_utils.hpp"

// Libraries <standard C++>
#include <atomic>
#include <array>
#include <cstring>
#include <cstdio>
#include <cwchar>

// Libraries <Windows.h> — before other Windows headers
#include <Windows.h>

// Libraries <Windows>
#include <evntrace.h>
#include <evntprov.h>

#include <MinHook.h>

namespace {

// PCLStats provider GUID (Streamline pclstats.h — PCLStatsTraceLoggingProvider)
static constexpr GUID kPCLStatsProviderId = {
    0x0d216f06, 0x82a6, 0x4d49, {0xbc, 0x4f, 0x8f, 0x38, 0xae, 0x56, 0xef, 0xab}};

static bool GuidEquals(const GUID* a, const GUID* b) { return std::memcmp(a, b, sizeof(GUID)) == 0; }

using EventRegister_pfn = decltype(&EventRegister);
using EventWriteTransfer_pfn = decltype(&EventWriteTransfer);

static EventRegister_pfn EventRegister_Original = nullptr;
static EventWriteTransfer_pfn EventWriteTransfer_Original = nullptr;

static std::atomic<bool> g_pclstats_etw_hooks_installed{false};
// REGHANDLE returned when a non–Display Commander module registers the PCLStats provider
static std::atomic<REGHANDLE> g_pclstats_foreign_reg_handle{REGHANDLE(0)};
static std::atomic<bool> g_pclstats_foreign_pclstatsinit_seen{false};
static std::atomic<ULONG> g_dc_cleanup_removed_count{0};
static std::atomic<ULONG> g_dc_cleanup_status{ERROR_GEN_FAILURE};
static wchar_t g_owned_dc_session_name_wide[64] = {};
static char g_owned_dc_session_name_utf8[64] = {};

static constexpr const char kPclStatsInitName[] = "PCLStatsInit";
static constexpr ULONG kPclStatsInitNameLen = 12;
static constexpr wchar_t kDcSessionNeedle[] = L"DC_";
static constexpr wchar_t kDisplayCommanderSessionPrefix[] = L"DisplayCommander_";

struct QueryEtwSessionProps {
    EVENT_TRACE_PROPERTIES props{};
    wchar_t logger_name[128]{};
    wchar_t log_file_name[2]{};
};

// Scan blob for PCLStatsInit (TraceLogging event name in descriptor payload)
static bool BlobContainsPclStatsInit(const void* ptr, ULONG size) {
    if (ptr == nullptr || size < kPclStatsInitNameLen) return false;
    const auto* p = static_cast<const unsigned char*>(ptr);
    const auto* end = p + size;
    for (const auto* cur = p; cur + kPclStatsInitNameLen <= end; ++cur) {
        if (std::memcmp(cur, kPclStatsInitName, kPclStatsInitNameLen) == 0) return true;
    }
    return false;
}

static bool IsDcNamedSession(const wchar_t* name) {
    if (name == nullptr || name[0] == L'\0') {
        return false;
    }
    if (std::wcsstr(name, kDcSessionNeedle) != nullptr) {
        return true;
    }
    const std::size_t prefix_len = std::wcslen(kDisplayCommanderSessionPrefix);
    return std::wcsncmp(name, kDisplayCommanderSessionPrefix, prefix_len) == 0;
}

static ULONG StopEtwSessionByName(const wchar_t* session_name) {
    if (session_name == nullptr || session_name[0] == L'\0') {
        return ERROR_INVALID_PARAMETER;
    }

    QueryEtwSessionProps stop_props{};
    stop_props.props.Wnode.BufferSize = sizeof(QueryEtwSessionProps);
    stop_props.props.LoggerNameOffset = static_cast<ULONG>(offsetof(QueryEtwSessionProps, logger_name));
    stop_props.props.LogFileNameOffset = static_cast<ULONG>(offsetof(QueryEtwSessionProps, log_file_name));
    std::wcsncpy(stop_props.logger_name, session_name, _countof(stop_props.logger_name) - 1);
    return ControlTraceW(0, stop_props.logger_name, &stop_props.props, EVENT_TRACE_CONTROL_STOP);
}

static void CleanupOtherDcEtwSessionsOnStartup() {
    constexpr ULONG kMaxSessions = 64;
    std::array<QueryEtwSessionProps, kMaxSessions> query_props{};
    std::array<EVENT_TRACE_PROPERTIES*, kMaxSessions> props_ptrs{};
    for (ULONG i = 0; i < kMaxSessions; ++i) {
        auto& p = query_props[i];
        p.props.Wnode.BufferSize = sizeof(QueryEtwSessionProps);
        p.props.LoggerNameOffset = static_cast<ULONG>(offsetof(QueryEtwSessionProps, logger_name));
        p.props.LogFileNameOffset = static_cast<ULONG>(offsetof(QueryEtwSessionProps, log_file_name));
        props_ptrs[i] = &p.props;
    }

    ULONG session_count = kMaxSessions;
    const ULONG query_status = QueryAllTracesW(props_ptrs.data(), kMaxSessions, &session_count);
    g_dc_cleanup_status.store(query_status, std::memory_order_release);
    if (query_status != ERROR_SUCCESS) {
        LogInfo("[PCLStats ETW] startup cleanup: QueryAllTracesW failed (status=%lu)", query_status);
        return;
    }

    ULONG removed = 0;
    for (ULONG i = 0; i < session_count; ++i) {
        const wchar_t* name = query_props[i].logger_name;
        if (!IsDcNamedSession(name)) {
            continue;
        }
        if (std::wcscmp(name, g_owned_dc_session_name_wide) == 0) {
            continue;
        }

        const ULONG stop_status = StopEtwSessionByName(name);
        if (stop_status == ERROR_SUCCESS) {
            ++removed;
            LogInfo("[PCLStats ETW] startup cleanup: stopped stale DC_ session '%s'",
                    display_commander::utils::WideToUtf8(name).c_str());
        } else {
            LogInfo("[PCLStats ETW] startup cleanup: failed to stop session '%s' (status=%lu)",
                    display_commander::utils::WideToUtf8(name).c_str(), stop_status);
        }
    }

    g_dc_cleanup_removed_count.store(removed, std::memory_order_release);
    LogInfo("[PCLStats ETW] startup cleanup done: removed %u stale DC_ session(s)", removed);
}

ULONG WINAPI EventRegister_Detour(LPCGUID ProviderId, PENABLECALLBACK EnableCallback, PVOID CallbackContext,
                                  PREGHANDLE RegHandle) {
    const ULONG ret = EventRegister_Original(ProviderId, EnableCallback, CallbackContext, RegHandle);
    HMODULE calling_module = GetCallingDLL();
    HMODULE our_module = g_module.load(std::memory_order_relaxed);
    if (calling_module != nullptr && our_module != nullptr && calling_module == our_module) {
        return ret;
    }
    if (ret == 0 && RegHandle != nullptr && ProviderId != nullptr && GuidEquals(ProviderId, &kPCLStatsProviderId)) {
        g_pclstats_foreign_reg_handle.store(*RegHandle, std::memory_order_release);
        static bool first_log = true;
        if (first_log) {
            first_log = false;
            LogInfo("[PCLStats ETW] foreign EventRegister for PCLStatsTraceLoggingProvider (handle non-zero)");
        }
    }
    return ret;
}

ULONG WINAPI EventWriteTransfer_Detour(REGHANDLE RegHandle, PCEVENT_DESCRIPTOR EventDescriptor, LPCGUID ActivityId,
                                       LPCGUID RelatedActivityId, ULONG UserDataCount,
                                       PEVENT_DATA_DESCRIPTOR UserData) {
    HMODULE calling_module = GetCallingDLL();
    HMODULE our_module = g_module.load(std::memory_order_relaxed);
    if (calling_module != nullptr && our_module != nullptr && calling_module == our_module) {
        return EventWriteTransfer_Original(RegHandle, EventDescriptor, ActivityId, RelatedActivityId, UserDataCount,
                                           UserData);
    }

    if (UserData != nullptr && UserDataCount > 0) {
        for (ULONG i = 0; i < UserDataCount; ++i) {
            const EVENT_DATA_DESCRIPTOR* d = &UserData[i];
            if (d->Size == 0 || d->Size > 0x10000) continue;
            const void* ptr = reinterpret_cast<const void*>(static_cast<uintptr_t>(d->Ptr));
            __try {
                if (BlobContainsPclStatsInit(ptr, d->Size)) {
                    g_pclstats_foreign_pclstatsinit_seen.store(true, std::memory_order_release);
                    static bool first_init_log = true;
                    if (first_init_log) {
                        first_init_log = false;
                        LogInfo("[PCLStats ETW] observed foreign PCLStatsInit (EventWriteTransfer)");
                    }
                    break;
                }
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                continue;
            }
        }
    }

    (void)EventDescriptor;
    (void)ActivityId;
    (void)RelatedActivityId;
    (void)RegHandle;

    return EventWriteTransfer_Original(RegHandle, EventDescriptor, ActivityId, RelatedActivityId, UserDataCount,
                                       UserData);
}

}  // namespace

bool InstallPCLStatsEtwHooks(HMODULE hModule) {
    if (g_pclstats_etw_hooks_installed.load(std::memory_order_acquire)) {
        return true;
    }
    if (hModule == nullptr) {
        LogInfo("[PCLStats ETW] InstallPCLStatsEtwHooks: null module");
        return false;
    }
    auto* pEventRegister = reinterpret_cast<EventRegister_pfn>(GetProcAddress(hModule, "EventRegister"));
    auto* pEventWriteTransfer = reinterpret_cast<EventWriteTransfer_pfn>(GetProcAddress(hModule, "EventWriteTransfer"));
    if (pEventRegister == nullptr || pEventWriteTransfer == nullptr) {
        LogInfo("[PCLStats ETW] EventRegister or EventWriteTransfer not found in advapi32");
        return false;
    }
    if (!CreateAndEnableHook(reinterpret_cast<LPVOID>(pEventRegister), reinterpret_cast<LPVOID>(&EventRegister_Detour),
                             reinterpret_cast<LPVOID*>(&EventRegister_Original), "EventRegister (PCLStats ETW)")) {
        return false;
    }
    if (!CreateAndEnableHook(reinterpret_cast<LPVOID>(pEventWriteTransfer),
                             reinterpret_cast<LPVOID>(&EventWriteTransfer_Detour),
                             reinterpret_cast<LPVOID*>(&EventWriteTransfer_Original), "EventWriteTransfer (PCLStats ETW)")) {
        MH_DisableHook(pEventRegister);
        MH_RemoveHook(pEventRegister);
        EventRegister_Original = nullptr;
        return false;
    }

    const DWORD pid = GetCurrentProcessId();
    std::swprintf(g_owned_dc_session_name_wide, _countof(g_owned_dc_session_name_wide), L"DC_PCLStats_%lu", pid);
    std::snprintf(g_owned_dc_session_name_utf8, sizeof(g_owned_dc_session_name_utf8), "DC_PCLStats_%lu", pid);
    CleanupOtherDcEtwSessionsOnStartup();

    g_pclstats_etw_hooks_installed.store(true, std::memory_order_release);
    LogInfo("[PCLStats ETW] hooks installed on advapi32 (foreign init detection)");
    return true;
}

void UninstallPCLStatsEtwHooks() {
    if (!g_pclstats_etw_hooks_installed.exchange(false, std::memory_order_acq_rel)) {
        return;
    }
    if (EventWriteTransfer_Original != nullptr) {
        MH_DisableHook(EventWriteTransfer_Original);
        MH_RemoveHook(EventWriteTransfer_Original);
        EventWriteTransfer_Original = nullptr;
    }
    if (EventRegister_Original != nullptr) {
        MH_DisableHook(EventRegister_Original);
        MH_RemoveHook(EventRegister_Original);
        EventRegister_Original = nullptr;
    }
    g_pclstats_foreign_reg_handle.store(REGHANDLE(0), std::memory_order_relaxed);
    g_pclstats_foreign_pclstatsinit_seen.store(false, std::memory_order_relaxed);
    g_dc_cleanup_removed_count.store(0, std::memory_order_relaxed);
    g_dc_cleanup_status.store(ERROR_GEN_FAILURE, std::memory_order_relaxed);
    g_owned_dc_session_name_wide[0] = L'\0';
    g_owned_dc_session_name_utf8[0] = '\0';
    LogInfo("[PCLStats ETW] hooks uninstalled");
}

bool ArePCLStatsEtwHooksInstalled() { return g_pclstats_etw_hooks_installed.load(std::memory_order_acquire); }

bool PclStatsForeignInitObserved() {
    if (g_pclstats_foreign_reg_handle.load(std::memory_order_acquire) != REGHANDLE(0)) {
        return true;
    }
    return g_pclstats_foreign_pclstatsinit_seen.load(std::memory_order_acquire);
}

const char* GetPCLStatsOwnedEtwSessionName() { return g_owned_dc_session_name_utf8; }

ULONG GetPCLStatsDcEtwCleanupRemovedCount() { return g_dc_cleanup_removed_count.load(std::memory_order_acquire); }

ULONG GetPCLStatsDcEtwCleanupStatus() { return g_dc_cleanup_status.load(std::memory_order_acquire); }
