#pragma once

namespace nvapi {

// Throttled NVAPI sampling (~150 ms) for overlay use; cached values are atomic.
// Call from the performance overlay when the option is enabled.
void PollGpuDynamicUtilizationForOverlay();

// Last good sample from PollGpuDynamicUtilizationForOverlay (0-100). Returns false if never sampled successfully.
bool GetCachedGpuDynamicUtilizationPercent(unsigned& out_percent);

}  // namespace nvapi
