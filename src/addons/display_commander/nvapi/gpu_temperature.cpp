// Source Code <Display Commander>
#include "gpu_temperature.hpp"
#include "../globals.hpp"
#include "../settings/main_tab_settings.hpp"
#include "nvapi_init.hpp"
#include "nvapi_loader.hpp"

#include <algorithm>
#include <atomic>
#include <cstdint>

#include <nvapi.h>

namespace {

constexpr NvU32 kNvapiThermalTargetGpu = 1;
constexpr NvU32 kNvapiThermalTargetAll = 15;
constexpr std::size_t kNvapiMaxThermalSensorsPerGpu = 3;

struct NvGpuThermalSensor {
    NvU32 controller;
    NvU32 default_min_temp;
    NvU32 default_max_temp;
    NvU32 current_temp;
    NvU32 target;
};

struct NvGpuThermalSettingsV2 {
    NvU32 version;
    NvU32 count;
    NvGpuThermalSensor sensor[kNvapiMaxThermalSensorsPerGpu];
};

constexpr NvU32 kNvGpuThermalSettingsV2Version = MAKE_NVAPI_VERSION(NvGpuThermalSettingsV2, 2);

// Only honor overlay requests for this many frames after the request frame id was set.
constexpr uint64_t kMaxFramesAfterRequest = 100;
// At most one NVAPI query per this many game frames.
constexpr uint64_t kMinFramesBetweenQueries = 100;

std::atomic<std::uint32_t> g_cached_temp_celsius{0};
std::atomic<bool> g_cache_valid{false};

bool QueryGpuTemperatureCelsius(unsigned& out_celsius) {
    out_celsius = 0;
    if (!nvapi::EnsureNvApiInitialized()) {
        return false;
    }
    const display_commander::nvapi_loader::NvApiPtrs* p = display_commander::nvapi_loader::Ptrs();
    if (p == nullptr || p->EnumPhysicalGPUs == nullptr || p->GPU_GetThermalSettings == nullptr) {
        return false;
    }

    NvPhysicalGpuHandle gpus[NVAPI_MAX_PHYSICAL_GPUS]{};
    NvU32 gpu_count = 0;
    if (p->EnumPhysicalGPUs(gpus, &gpu_count) != NVAPI_OK || gpu_count == 0) {
        return false;
    }

    NvGpuThermalSettingsV2 settings{};
    settings.version = kNvGpuThermalSettingsV2Version;
    if (p->GPU_GetThermalSettings(gpus[0], kNvapiThermalTargetAll, &settings) != NVAPI_OK) {
        return false;
    }

    const std::size_t sensor_count = (std::min)(static_cast<std::size_t>(settings.count), kNvapiMaxThermalSensorsPerGpu);
    if (sensor_count == 0) {
        return false;
    }

    for (std::size_t i = 0; i < sensor_count; ++i) {
        const NvGpuThermalSensor& s = settings.sensor[i];
        if (s.target == kNvapiThermalTargetGpu) {
            out_celsius = static_cast<unsigned>(s.current_temp);
            return true;
        }
    }

    out_celsius = static_cast<unsigned>(settings.sensor[0].current_temp);
    return true;
}

}  // namespace

namespace nvapi {

void RequestGpuTemperatureFromOverlay(bool enabled) {
    if (!enabled) {
        g_nvapi_gpu_temp_request_frame_id.store(0, std::memory_order_relaxed);
        return;
    }
    g_nvapi_gpu_temp_request_frame_id.store(g_global_frame_id.load(std::memory_order_relaxed), std::memory_order_relaxed);
}

void ProcessGpuTemperatureRequestInContinuousMonitoring() {
    if (!settings::g_mainTabSettings.show_overlay_nvapi_gpu_temp.GetValue()) {
        g_nvapi_gpu_temp_request_frame_id.store(0, std::memory_order_relaxed);
        return;
    }

    const uint64_t req = g_nvapi_gpu_temp_request_frame_id.load(std::memory_order_acquire);
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

    const uint64_t last_q = g_nvapi_gpu_temp_last_query_frame_id.load(std::memory_order_relaxed);
    if (last_q != 0 && cur - last_q < kMinFramesBetweenQueries) {
        return;
    }

    unsigned t = 0;
    if (QueryGpuTemperatureCelsius(t)) {
        g_cached_temp_celsius.store(static_cast<std::uint32_t>(t), std::memory_order_relaxed);
        g_cache_valid.store(true, std::memory_order_relaxed);
    }
    // On failure, keep previous cache; still advance last_query_frame to avoid hammering NVAPI.
    g_nvapi_gpu_temp_last_query_frame_id.store(cur, std::memory_order_relaxed);
}

bool GetCachedGpuTemperatureCelsius(unsigned& out_celsius) {
    if (!g_cache_valid.load(std::memory_order_relaxed)) {
        return false;
    }
    out_celsius = static_cast<unsigned>(g_cached_temp_celsius.load(std::memory_order_relaxed));
    return true;
}

}  // namespace nvapi
