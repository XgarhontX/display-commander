#pragma once

// Source Code <Display Commander> // follow this order for includes in all files + add this comment at the top

// Libraries <standard C++>
#include <cstdint>

// Libraries <Windows.h> — before other Windows headers
#include <Windows.h>

// Libraries <Windows>
#include <dxgi.h>

namespace display_commander::features::streamline {

/** Session counters for each Streamline proxy DXGI detour (debug UI). */
enum class StreamlineProxyDxgiDetourCounter : int {
    Factory_CreateSwapChain = 0,
    Factory1_CreateSwapChainForHwnd,
    Factory1_CreateSwapChainForCoreWindow,
    Factory2_CreateSwapChainForComposition,
    SwapChain_Present,
    SwapChain1_Present1,
    Count_
};

uint64_t GetStreamlineProxyDxgiDetourCallCount(StreamlineProxyDxgiDetourCounter which);
const char* GetStreamlineProxyDxgiDetourCounterLabel(StreamlineProxyDxgiDetourCounter which);
void ResetStreamlineProxyDxgiDetourCallCounts();

/** MinHook Streamline proxy DXGI factory vtable (CreateSwapChain*); idempotent. */
bool HookStreamlineProxyFactory(IUnknown* upgraded_iface);

/** MinHook Present/Present1 on a Streamline proxy swap chain for FPS limiter path; idempotent per vtable. */
bool HookStreamlineProxySwapchain(IDXGISwapChain* swapchain);

}  // namespace display_commander::features::streamline
