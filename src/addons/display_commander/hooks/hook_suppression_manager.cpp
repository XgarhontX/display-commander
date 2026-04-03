#include "hook_suppression_manager.hpp"
#include <atomic>
#include <cstdint>
#include <set>
#include "../settings/hook_suppression_settings.hpp"
#include "../utils/logging.hpp"

namespace display_commanderhooks {

HookSuppressionManager& HookSuppressionManager::GetInstance() {
    static HookSuppressionManager instance;
    return instance;
}

namespace {
// Runtime-only mirror of which hook families reported successful install (not written to config).
std::atomic<uint32_t> g_hooks_installed_bits{0};

// Helper function to get the setting reference for a hook type
ui::new_ui::SettingBase* GetSuppressionSetting(HookType hookType) {
    switch (hookType) {
        case HookType::DXGI_FACTORY:   return &settings::g_hook_suppression_settings.suppress_dxgi_factory_hooks;
        case HookType::DXGI_SWAPCHAIN: return &settings::g_hook_suppression_settings.suppress_dxgi_swapchain_hooks;
        case HookType::SL_PROXY_DXGI_SWAPCHAIN:
            return &settings::g_hook_suppression_settings.suppress_sl_proxy_dxgi_swapchain_hooks;
        case HookType::XINPUT:       return &settings::g_hook_suppression_settings.suppress_xinput_hooks;
        case HookType::STREAMLINE:   return &settings::g_hook_suppression_settings.suppress_streamline_hooks;
        case HookType::NGX:          return &settings::g_hook_suppression_settings.suppress_ngx_hooks;
        case HookType::WINDOWS_GAMING_INPUT:
            return &settings::g_hook_suppression_settings.suppress_windows_gaming_input_hooks;
        case HookType::API:              return &settings::g_hook_suppression_settings.suppress_api_hooks;
        case HookType::WINDOW_API:       return &settings::g_hook_suppression_settings.suppress_window_api_hooks;
        case HookType::TIMESLOWDOWN:     return &settings::g_hook_suppression_settings.suppress_timeslowdown_hooks;
        case HookType::DEBUG_OUTPUT:     return &settings::g_hook_suppression_settings.suppress_debug_output_hooks;
        case HookType::LOADLIBRARY:      return &settings::g_hook_suppression_settings.suppress_loadlibrary_hooks;
        case HookType::DISPLAY_SETTINGS: return &settings::g_hook_suppression_settings.suppress_display_settings_hooks;
        case HookType::WINDOWS_MESSAGE:  return &settings::g_hook_suppression_settings.suppress_windows_message_hooks;
        case HookType::OPENGL:           return &settings::g_hook_suppression_settings.suppress_opengl_hooks;
        case HookType::NVAPI:            return &settings::g_hook_suppression_settings.suppress_nvapi_hooks;
        case HookType::PROCESS_EXIT:     return &settings::g_hook_suppression_settings.suppress_process_exit_hooks;
        case HookType::VULKAN_LOADER:     return &settings::g_hook_suppression_settings.suppress_vulkan_loader_hooks;
        default:                         return nullptr;
    }
}
}  // anonymous namespace

bool HookSuppressionManager::ShouldSuppressHook(HookType hookType) {
    // Log suppression hint once per hook type (only if setting is not already enabled)
    static std::set<HookType> logged_hook_types;
    if (!logged_hook_types.contains(hookType)) {
        ui::new_ui::SettingBase* setting = GetSuppressionSetting(hookType);
        if (setting != nullptr) {
            // Get current value to check if already suppressed
            bool current_value = false;
            switch (hookType) {
                case HookType::DXGI_FACTORY:
                    current_value = settings::g_hook_suppression_settings.suppress_dxgi_factory_hooks.GetValue();
                    break;
                case HookType::DXGI_SWAPCHAIN:
                    current_value = settings::g_hook_suppression_settings.suppress_dxgi_swapchain_hooks.GetValue();
                    break;
                case HookType::SL_PROXY_DXGI_SWAPCHAIN:
                    current_value =
                        settings::g_hook_suppression_settings.suppress_sl_proxy_dxgi_swapchain_hooks.GetValue();
                    break;
                case HookType::XINPUT:
                    current_value = settings::g_hook_suppression_settings.suppress_xinput_hooks.GetValue();
                    break;
                case HookType::STREAMLINE:
                    current_value = settings::g_hook_suppression_settings.suppress_streamline_hooks.GetValue();
                    break;
                case HookType::NGX:
                    current_value = settings::g_hook_suppression_settings.suppress_ngx_hooks.GetValue();
                    break;
                case HookType::WINDOWS_GAMING_INPUT:
                    current_value =
                        settings::g_hook_suppression_settings.suppress_windows_gaming_input_hooks.GetValue();
                    break;
                case HookType::API:
                    current_value = settings::g_hook_suppression_settings.suppress_api_hooks.GetValue();
                    break;
                case HookType::WINDOW_API:
                    current_value = settings::g_hook_suppression_settings.suppress_window_api_hooks.GetValue();
                    break;
                case HookType::TIMESLOWDOWN:
                    current_value = settings::g_hook_suppression_settings.suppress_timeslowdown_hooks.GetValue();
                    break;
                case HookType::DEBUG_OUTPUT:
                    current_value = settings::g_hook_suppression_settings.suppress_debug_output_hooks.GetValue();
                    break;
                case HookType::LOADLIBRARY:
                    current_value = settings::g_hook_suppression_settings.suppress_loadlibrary_hooks.GetValue();
                    break;
                case HookType::DISPLAY_SETTINGS:
                    current_value = settings::g_hook_suppression_settings.suppress_display_settings_hooks.GetValue();
                    break;
                case HookType::WINDOWS_MESSAGE:
                    current_value = settings::g_hook_suppression_settings.suppress_windows_message_hooks.GetValue();
                    break;
                case HookType::OPENGL:
                    current_value = settings::g_hook_suppression_settings.suppress_opengl_hooks.GetValue();
                    break;
                case HookType::NVAPI:
                    current_value = settings::g_hook_suppression_settings.suppress_nvapi_hooks.GetValue();
                    break;
                case HookType::PROCESS_EXIT:
                    current_value = settings::g_hook_suppression_settings.suppress_process_exit_hooks.GetValue();
                    break;
                case HookType::VULKAN_LOADER:
                    current_value = settings::g_hook_suppression_settings.suppress_vulkan_loader_hooks.GetValue();
                    break;
                default: break;
            }

            if (!current_value) {
                LogInfo("To suppress %s hooks, set %s=1 in [%s] section of DisplayCommander.ini",
                        GetHookTypeName(hookType).c_str(), setting->GetKey().c_str(), setting->GetSection().c_str());
            }
            logged_hook_types.insert(hookType);
        }
    }

    switch (hookType) {
        case HookType::DXGI_FACTORY:
            return settings::g_hook_suppression_settings.suppress_dxgi_factory_hooks.GetValue();
        case HookType::DXGI_SWAPCHAIN:
            return settings::g_hook_suppression_settings.suppress_dxgi_swapchain_hooks.GetValue();
        case HookType::SL_PROXY_DXGI_SWAPCHAIN:
            return settings::g_hook_suppression_settings.suppress_sl_proxy_dxgi_swapchain_hooks.GetValue();
        case HookType::XINPUT:     return settings::g_hook_suppression_settings.suppress_xinput_hooks.GetValue();
        case HookType::STREAMLINE: return settings::g_hook_suppression_settings.suppress_streamline_hooks.GetValue();
        case HookType::NGX:        return settings::g_hook_suppression_settings.suppress_ngx_hooks.GetValue();
        case HookType::WINDOWS_GAMING_INPUT:
            return settings::g_hook_suppression_settings.suppress_windows_gaming_input_hooks.GetValue();
        case HookType::API:         return settings::g_hook_suppression_settings.suppress_api_hooks.GetValue();
        case HookType::WINDOW_API:  return settings::g_hook_suppression_settings.suppress_window_api_hooks.GetValue();
        case HookType::TIMESLOWDOWN:
            return settings::g_hook_suppression_settings.suppress_timeslowdown_hooks.GetValue();
        case HookType::DEBUG_OUTPUT:
            return settings::g_hook_suppression_settings.suppress_debug_output_hooks.GetValue();
        case HookType::LOADLIBRARY: return settings::g_hook_suppression_settings.suppress_loadlibrary_hooks.GetValue();
        case HookType::DISPLAY_SETTINGS:
            return settings::g_hook_suppression_settings.suppress_display_settings_hooks.GetValue();
        case HookType::WINDOWS_MESSAGE:
            return settings::g_hook_suppression_settings.suppress_windows_message_hooks.GetValue();
        case HookType::OPENGL: return settings::g_hook_suppression_settings.suppress_opengl_hooks.GetValue();
        case HookType::NVAPI: return settings::g_hook_suppression_settings.suppress_nvapi_hooks.GetValue();
        case HookType::PROCESS_EXIT:
            return settings::g_hook_suppression_settings.suppress_process_exit_hooks.GetValue();
        case HookType::VULKAN_LOADER:
            return settings::g_hook_suppression_settings.suppress_vulkan_loader_hooks.GetValue();
        default:
            LogError("HookSuppressionManager::ShouldSuppressHook - Invalid hook type: %d", static_cast<int>(hookType));
            return false;
    }
}

void HookSuppressionManager::SetSuppressHook(HookType hookType, bool suppress) {
    switch (hookType) {
        case HookType::DXGI_FACTORY:
            settings::g_hook_suppression_settings.suppress_dxgi_factory_hooks.SetValue(suppress);
            settings::g_hook_suppression_settings.suppress_dxgi_factory_hooks.Save();
            break;
        case HookType::DXGI_SWAPCHAIN:
            settings::g_hook_suppression_settings.suppress_dxgi_swapchain_hooks.SetValue(suppress);
            settings::g_hook_suppression_settings.suppress_dxgi_swapchain_hooks.Save();
            break;
        case HookType::SL_PROXY_DXGI_SWAPCHAIN:
            settings::g_hook_suppression_settings.suppress_sl_proxy_dxgi_swapchain_hooks.SetValue(suppress);
            settings::g_hook_suppression_settings.suppress_sl_proxy_dxgi_swapchain_hooks.Save();
            break;
        case HookType::XINPUT:
            settings::g_hook_suppression_settings.suppress_xinput_hooks.SetValue(suppress);
            settings::g_hook_suppression_settings.suppress_xinput_hooks.Save();
            break;
        case HookType::STREAMLINE:
            settings::g_hook_suppression_settings.suppress_streamline_hooks.SetValue(suppress);
            settings::g_hook_suppression_settings.suppress_streamline_hooks.Save();
            break;
        case HookType::NGX:
            settings::g_hook_suppression_settings.suppress_ngx_hooks.SetValue(suppress);
            settings::g_hook_suppression_settings.suppress_ngx_hooks.Save();
            break;
        case HookType::WINDOWS_GAMING_INPUT:
            settings::g_hook_suppression_settings.suppress_windows_gaming_input_hooks.SetValue(suppress);
            settings::g_hook_suppression_settings.suppress_windows_gaming_input_hooks.Save();
            break;
        case HookType::API:
            settings::g_hook_suppression_settings.suppress_api_hooks.SetValue(suppress);
            settings::g_hook_suppression_settings.suppress_api_hooks.Save();
            break;
        case HookType::WINDOW_API:
            settings::g_hook_suppression_settings.suppress_window_api_hooks.SetValue(suppress);
            settings::g_hook_suppression_settings.suppress_window_api_hooks.Save();
            break;
        case HookType::TIMESLOWDOWN:
            settings::g_hook_suppression_settings.suppress_timeslowdown_hooks.SetValue(suppress);
            settings::g_hook_suppression_settings.suppress_timeslowdown_hooks.Save();
            break;
        case HookType::DEBUG_OUTPUT:
            settings::g_hook_suppression_settings.suppress_debug_output_hooks.SetValue(suppress);
            settings::g_hook_suppression_settings.suppress_debug_output_hooks.Save();
            break;
        case HookType::LOADLIBRARY:
            settings::g_hook_suppression_settings.suppress_loadlibrary_hooks.SetValue(suppress);
            settings::g_hook_suppression_settings.suppress_loadlibrary_hooks.Save();
            break;
        case HookType::DISPLAY_SETTINGS:
            settings::g_hook_suppression_settings.suppress_display_settings_hooks.SetValue(suppress);
            settings::g_hook_suppression_settings.suppress_display_settings_hooks.Save();
            break;
        case HookType::WINDOWS_MESSAGE:
            settings::g_hook_suppression_settings.suppress_windows_message_hooks.SetValue(suppress);
            settings::g_hook_suppression_settings.suppress_windows_message_hooks.Save();
            break;
        case HookType::OPENGL:
            settings::g_hook_suppression_settings.suppress_opengl_hooks.SetValue(suppress);
            settings::g_hook_suppression_settings.suppress_opengl_hooks.Save();
            break;
        case HookType::NVAPI:
            settings::g_hook_suppression_settings.suppress_nvapi_hooks.SetValue(suppress);
            settings::g_hook_suppression_settings.suppress_nvapi_hooks.Save();
            break;
        case HookType::PROCESS_EXIT:
            settings::g_hook_suppression_settings.suppress_process_exit_hooks.SetValue(suppress);
            settings::g_hook_suppression_settings.suppress_process_exit_hooks.Save();
            break;
        case HookType::VULKAN_LOADER:
            settings::g_hook_suppression_settings.suppress_vulkan_loader_hooks.SetValue(suppress);
            settings::g_hook_suppression_settings.suppress_vulkan_loader_hooks.Save();
            break;
        default:
            LogError("HookSuppressionManager::SetSuppressHook - Invalid hook type: %d", static_cast<int>(hookType));
            break;
    }
}

void HookSuppressionManager::MarkHookInstalled(HookType hookType) {
    const int type_index = static_cast<int>(hookType);
    if (type_index < 0 || type_index >= kHookTypeCount) {
        LogError("HookSuppressionManager::MarkHookInstalled - Invalid hook type: %d", type_index);
        return;
    }

    const uint32_t mask = 1u << static_cast<unsigned>(type_index);
    const uint32_t prev = g_hooks_installed_bits.fetch_or(mask, std::memory_order_relaxed);
    if ((prev & mask) == 0) {
        switch (hookType) {
            case HookType::DXGI_FACTORY:
                settings::g_hook_suppression_settings.suppress_dxgi_factory_hooks.Save();
                break;
            case HookType::DXGI_SWAPCHAIN:
                settings::g_hook_suppression_settings.suppress_dxgi_swapchain_hooks.Save();
                break;
            case HookType::SL_PROXY_DXGI_SWAPCHAIN:
                settings::g_hook_suppression_settings.suppress_sl_proxy_dxgi_swapchain_hooks.Save();
                break;
            case HookType::XINPUT:
                settings::g_hook_suppression_settings.suppress_xinput_hooks.Save();
                break;
            case HookType::STREAMLINE:
                settings::g_hook_suppression_settings.suppress_streamline_hooks.Save();
                break;
            case HookType::NGX:
                settings::g_hook_suppression_settings.suppress_ngx_hooks.Save();
                break;
            case HookType::WINDOWS_GAMING_INPUT:
                settings::g_hook_suppression_settings.suppress_windows_gaming_input_hooks.Save();
                break;
            case HookType::API:
                settings::g_hook_suppression_settings.suppress_api_hooks.Save();
                break;
            case HookType::WINDOW_API:
                settings::g_hook_suppression_settings.suppress_window_api_hooks.Save();
                break;
            case HookType::TIMESLOWDOWN:
                settings::g_hook_suppression_settings.suppress_timeslowdown_hooks.Save();
                break;
            case HookType::DEBUG_OUTPUT:
                settings::g_hook_suppression_settings.suppress_debug_output_hooks.Save();
                break;
            case HookType::LOADLIBRARY:
                settings::g_hook_suppression_settings.suppress_loadlibrary_hooks.Save();
                break;
            case HookType::DISPLAY_SETTINGS:
                settings::g_hook_suppression_settings.suppress_display_settings_hooks.Save();
                break;
            case HookType::WINDOWS_MESSAGE:
                settings::g_hook_suppression_settings.suppress_windows_message_hooks.Save();
                break;
            case HookType::OPENGL:
                settings::g_hook_suppression_settings.suppress_opengl_hooks.Save();
                break;
            case HookType::NVAPI:
                settings::g_hook_suppression_settings.suppress_nvapi_hooks.Save();
                break;
            case HookType::PROCESS_EXIT:
                settings::g_hook_suppression_settings.suppress_process_exit_hooks.Save();
                break;
            case HookType::VULKAN_LOADER:
                settings::g_hook_suppression_settings.suppress_vulkan_loader_hooks.Save();
                break;
            default:
                LogError("HookSuppressionManager::MarkHookInstalled - Invalid hook type: %d", type_index);
                break;
        }
    }

    LogInfo("HookSuppressionManager::MarkHookInstalled - Marked %d as installed", type_index);
}

std::string HookSuppressionManager::GetHookTypeName(HookType hookType) {
    switch (hookType) {
        case HookType::DXGI_FACTORY:            return "DXGI Factory";
        case HookType::DXGI_SWAPCHAIN:          return "DXGI Swapchain";
        case HookType::SL_PROXY_DXGI_SWAPCHAIN: return "SL Proxy DXGI Swapchain";
        case HookType::XINPUT:                  return "XInput";
        case HookType::STREAMLINE:              return "Streamline";
        case HookType::NGX:                     return "NGX";
        case HookType::WINDOWS_GAMING_INPUT:    return "Windows Gaming Input";
        case HookType::API:                     return "API";
        case HookType::WINDOW_API:              return "Window API";
        case HookType::TIMESLOWDOWN:            return "Time Slowdown";
        case HookType::DEBUG_OUTPUT:            return "Debug Output";
        case HookType::LOADLIBRARY:             return "LoadLibrary";
        case HookType::DISPLAY_SETTINGS:        return "Display Settings";
        case HookType::WINDOWS_MESSAGE:         return "Windows Message";
        case HookType::OPENGL:                  return "OpenGL";
        case HookType::NVAPI:                   return "NVAPI";
        case HookType::PROCESS_EXIT:            return "Process Exit";
        case HookType::VULKAN_LOADER:           return "Vulkan Loader";
        default:
            LogError("HookSuppressionManager::GetHookTypeName - Invalid hook type: %d", static_cast<int>(hookType));
            return "Unknown";
    }
}

bool HookSuppressionManager::IsHookInstalled(HookType hookType) {
    const int type_index = static_cast<int>(hookType);
    if (type_index < 0 || type_index >= kHookTypeCount) {
        return false;
    }
    const uint32_t mask = 1u << static_cast<unsigned>(type_index);
    return (g_hooks_installed_bits.load(std::memory_order_relaxed) & mask) != 0;
}

HookType HookSuppressionManager::GetHookTypeByIndex(int index) {
    if (index < 0 || index >= kHookTypeCount) {
        return static_cast<HookType>(0);
    }
    return static_cast<HookType>(index);
}

}  // namespace display_commanderhooks
