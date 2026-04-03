#pragma once

#define WIN32_LEAN_AND_MEAN
#include <dxgi1_3.h>
#include <dxgi1_6.h>
#include <windef.h>
#include <windows.h>
#include <wrl/client.h>
#include <reshade_imgui.hpp>

#include <cstdint>
#include <vector>

#include "config/override_reshade_settings.hpp"
#include "globals.hpp"
#include "reshade_event_registration.hpp"
#include "utils.hpp"

// WASAPI per-app volume control
#include <audiopolicy.h>
#include <mmdeviceapi.h>

// Command list and queue lifecycle hooks (declared in swapchain_events.hpp)

// Function declarations
void ApplyWindowChange(HWND hwnd, const char* reason = "unknown", bool force_apply = false);

// CONTINUOUS RENDERING FUNCTIONS REMOVED - Focus spoofing is now handled by Win32 hooks

// Swapchain event handlers (declared in swapchain_events.hpp)

// Power saving settings and swapchain utilities (declared in swapchain_events.hpp)

// Initialization functions
bool FinishAddonRegistration(HMODULE addon_module, HMODULE reshade_module, bool do_unregister = true);
