// Source Code <Display Commander>
#include "gpu_dynamic_utilization.hpp"
#include "../globals.hpp"
#include "../settings/main_tab_settings.hpp"
#include "nvapi_init.hpp"
#include "nvapi_loader.hpp"

#include <algorithm>
#include <atomic>
#include <cstdint>

#include <nvapi.h>

namespace {

// NVAPI documents utilization[0] as the GPU (graphics engine) domain.
constexpr std::size_t kGpuEngineUtilizationIndex = 0;

// Only honor overlay requests for this many frames after the request frame id was set.
constexpr uint64_t kMaxFramesAfterRequest = 100;
// At most one NVAPI query per this many game frames.
constexpr uint64_t kMinFramesBetweenQueries = 100;

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

void RequestGpuDynamicUtilizationFromOverlay(bool enabled) {
    if (!enabled) {
        g_nvapi_gpu_util_request_frame_id.store(0, std::memory_order_relaxed);
        return;
    }
    g_nvapi_gpu_util_request_frame_id.store(g_global_frame_id.load(std::memory_order_relaxed),
                                            std::memory_order_relaxed);
}

void ProcessGpuDynamicUtilizationRequestInContinuousMonitoring() {
    if (!settings::g_mainTabSettings.show_overlay_nvapi_gpu_util.GetValue()) {
        g_nvapi_gpu_util_request_frame_id.store(0, std::memory_order_relaxed);
        return;
    }

    const uint64_t req = g_nvapi_gpu_util_request_frame_id.load(std::memory_order_acquire);
    if (req == 0) {
        return;
    }

    const uint64_t cur = g_global_frame_id.load(std::memory_order_acquire);
    if (cur < req) {
        return;
    }
    if (cur - req > kMaxFramesAfterRequest) {
        return;
    }

    const uint64_t last_q = g_nvapi_gpu_util_last_query_frame_id.load(std::memory_order_relaxed);
    if (last_q != 0 && cur - last_q < kMinFramesBetweenQueries) {
        return;
    }

    unsigned p = 0;
    if (QueryGpuEngineUtilizationPercent(p)) {
        g_cached_percent.store(static_cast<std::uint32_t>(p), std::memory_order_relaxed);
        g_cache_valid.store(true, std::memory_order_relaxed);
    }
    // On failure, keep previous cache; still advance last_query_frame to avoid hammering NVAPI.
    g_nvapi_gpu_util_last_query_frame_id.store(cur, std::memory_order_relaxed);
}

bool GetCachedGpuDynamicUtilizationPercent(unsigned& out_percent) {
    if (!g_cache_valid.load(std::memory_order_relaxed)) {
        return false;
    }
    out_percent = static_cast<unsigned>(g_cached_percent.load(std::memory_order_relaxed));
    return true;
}

}  // namespace nvapi
