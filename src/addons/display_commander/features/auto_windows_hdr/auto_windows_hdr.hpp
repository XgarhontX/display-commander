// Source Code <Display Commander> // Auto Windows HDR feature slice; see features/auto_windows_hdr/
// Behavior: docs/spec/features/auto_enable_windows_hdr.md
#pragma once

// Libraries <Windows.h>
#include <Windows.h>

namespace display_commander::features::auto_windows_hdr {

// After settings load during DLL init (no-HWND path): best-effort HDR enable; see spec.
void OnEarlyInitTryAutoEnableWindowsHdr();

// After swapchain init (ReShade `init_swapchain`): enable Windows HDR for the game window’s monitor when the setting is on.
void OnSwapchainInitTryAutoEnableWindowsHdr(HWND hwnd);

// On swapchain destroy: if we auto-enabled HDR for this HWND’s monitor, turn HDR off and clear state.
void OnSwapchainDestroyMaybeRevertAutoHdr(HWND hwnd);

// On ReShade `destroy_device`: if we auto-enabled HDR this session, turn it off and clear state (no device/window match).
void OnDestroyDeviceRevertAutoHdrIfNeeded();

// On process exit: if we still believe we auto-enabled HDR, turn it off using the stored monitor (no HWND).
void OnProcessExitRevertAutoHdrIfNeeded();

}  // namespace display_commander::features::auto_windows_hdr
