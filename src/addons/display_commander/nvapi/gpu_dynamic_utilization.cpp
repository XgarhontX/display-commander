// Source Code <Display Commander>
#include "gpu_dynamic_utilization.hpp"
#include "nvapi_init.hpp"
#include "nvapi_loader.hpp"
#include "../utils/timing.hpp"

#include <algorithm>
#include <atomic>
#include <cstdint>

#include <nvapi.h>

namespace {

// NVAPI documents utilization[0] as the GPU (graphics engine) domain.
constexpr std::size_t kGpuEngineUtilizationIndex = 0;

// Driver rolls utilization over ~1 s; polling more often adds cost without much benefit.
constexpr LONGLONG kMinPollIntervalNs = static_cast<LONGLONG>(0.15 * static_cast<double>(utils::SEC_TO_NS));

std::atomic<LONGLONG> g_last_poll_ns{0};
std::atomic<std::uint32_t> g_cached_percent{0};
std::atomic<bool> g_cache_valid{false};

bool QueryGpuEngineUtilizationPercent(unsigned& out_percent) {
    out_percent = 0;
    if (!nvapi::EnsureNvApiInitialized()) {
        return false;
    }
    const display_commander::nvapi_loader::NvApiPtrs* p = display_commander::nvapi_loader::Ptrs();
    if (p == nullptr || p->EnumPhysicalGPUs == nullptr || p->GPU_GetDynamicPstatesInfoEx == nullptr) {
        return false;
    }

    NvPhysicalGpuHandle gpus[NVAPI_MAX_PHYSICAL_GPUS]{};
    NvU32 gpu_count = 0;
    if (p->EnumPhysicalGPUs(gpus, &gpu_count) != NVAPI_OK || gpu_count == 0) {
        return false;
    }

    NV_GPU_DYNAMIC_PSTATES_INFO_EX info{};
    info.version = NV_GPU_DYNAMIC_PSTATES_INFO_EX_VER;
    if (p->GPU_GetDynamicPstatesInfoEx(gpus[0], &info) != NVAPI_OK) {
        return false;
    }

    if (kGpuEngineUtilizationIndex >= NVAPI_MAX_GPU_UTILIZATIONS) {
        return false;
    }
    const auto& u = info.utilization[kGpuEngineUtilizationIndex];
    if (u.bIsPresent == 0) {
        return false;
    }
    const NvU32 pct = u.percentage;
    out_percent = static_cast<unsigned>((std::min)(pct, static_cast<NvU32>(100)));
    return true;
}

}  // namespace

namespace nvapi {

void PollGpuDynamicUtilizationForOverlay() {
    const LONGLONG now = utils::get_now_ns();
    LONGLONG last = g_last_poll_ns.load(std::memory_order_relaxed);
    if (now - last < kMinPollIntervalNs) {
        return;
    }
    if (!g_last_poll_ns.compare_exchange_strong(last, now, std::memory_order_relaxed)) {
        return;
    }

    unsigned p = 0;
    if (QueryGpuEngineUtilizationPercent(p)) {
        g_cached_percent.store(static_cast<std::uint32_t>(p), std::memory_order_relaxed);
        g_cache_valid.store(true, std::memory_order_relaxed);
    }
    // On failure, keep the previous cached value so the overlay does not flicker empty.
}

bool GetCachedGpuDynamicUtilizationPercent(unsigned& out_percent) {
    if (!g_cache_valid.load(std::memory_order_relaxed)) {
        return false;
    }
    out_percent = static_cast<unsigned>(g_cached_percent.load(std::memory_order_relaxed));
    return true;
}

}  // namespace nvapi
