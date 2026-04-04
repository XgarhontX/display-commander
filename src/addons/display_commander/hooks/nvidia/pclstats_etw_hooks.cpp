// Source Code <Display Commander> // follow this order for includes in all files + add this comment at the top
#include "pclstats_etw_hooks.hpp"
#include "../../globals.hpp"
#include "../../utils/general_utils.hpp"
#include "../../utils/logging.hpp"

// Libraries <standard C++>
#include <atomic>
#include <cstring>

// Libraries <Windows.h> — before other Windows headers
#include <Windows.h>

// Libraries <Windows>
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

static constexpr const char kPclStatsInitName[] = "PCLStatsInit";
static constexpr ULONG kPclStatsInitNameLen = 12;

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
    LogInfo("[PCLStats ETW] hooks uninstalled");
}

bool ArePCLStatsEtwHooksInstalled() { return g_pclstats_etw_hooks_installed.load(std::memory_order_acquire); }

bool PclStatsForeignInitObserved() {
    if (g_pclstats_foreign_reg_handle.load(std::memory_order_acquire) != REGHANDLE(0)) {
        return true;
    }
    return g_pclstats_foreign_pclstatsinit_seen.load(std::memory_order_acquire);
}
