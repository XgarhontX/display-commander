#pragma once

#include "../nvapi/reflex_manager.hpp"

// NVIDIA Reflex provider (single-technology; no LatencyManager abstraction).
class ReflexProvider {
   public:
    ReflexProvider();
    ~ReflexProvider();

    bool Initialize(reshade::api::device* device);
    bool InitializeNative(void* native_device, DeviceTypeDC device_type);
    void Shutdown();
    bool IsInitialized() const;

    bool SetMarker(NV_LATENCY_MARKER_TYPE marker);
    bool ApplySleepMode(bool low_latency, bool boost, bool use_markers, float fps_limit);
    bool Sleep();

    void UpdateCachedSleepStatus();
    bool GetSleepStatus(NV_GET_SLEEP_STATUS_PARAMS* status_params,
                       SleepStatusUnavailableReason* out_reason = nullptr);

    struct NvapiLatencyMetrics {
        double pc_latency_ms = 0.0;
        double gpu_frame_time_ms = 0.0;
        uint64_t frame_id = 0;
    };

    // Query NVAPI Reflex latency metrics (PC latency and GPU frame time) for the most recent frame.
    // Returns false when Reflex latency reporting is unavailable or on error.
    bool GetLatencyMetrics(NvapiLatencyMetrics& out_metrics);

    static void EnsurePCLStatsInitialized();
    static bool IsPCLStatsInitialized();

   private:
    ReflexManager reflex_manager_;
    static bool _is_pcl_initialized;
};
