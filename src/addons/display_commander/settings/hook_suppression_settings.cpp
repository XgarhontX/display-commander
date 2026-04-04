#include "hook_suppression_settings.hpp"
#include "../utils/logging.hpp"

namespace settings {

HookSuppressionSettings::HookSuppressionSettings()
    // Hook suppression settings - hooks with true are suppressed by default (blacklisted)
    // Most hooks are enabled by default (false) for normal operation
    // Only hooks that are typically not needed or can cause issues are blacklisted by default
    : suppress_dxgi_factory_hooks("DxgiFactoryHooks", false, "DisplayCommander.HookSuppression"),
      suppress_dxgi_swapchain_hooks("DxgiSwapchainHooks", false, "DisplayCommander.HookSuppression"),
      suppress_sl_proxy_dxgi_swapchain_hooks("SlProxyDxgiSwapchainHooks", false, "DisplayCommander.HookSuppression"),
      suppress_xinput_hooks("XInputHooks", false, "DisplayCommander.HookSuppression"),
      suppress_streamline_hooks("StreamlineHooks", false, "DisplayCommander.HookSuppression"),
      suppress_ngx_hooks("NGXHooks", false, "DisplayCommander.HookSuppression"),
      suppress_windows_gaming_input_hooks("WindowsGamingInputHooks", false, "DisplayCommander.HookSuppression"),
      suppress_api_hooks("ApiHooks", false, "DisplayCommander.HookSuppression"),
      suppress_window_api_hooks("WindowApiHooks", false, "DisplayCommander.HookSuppression"),
      suppress_timeslowdown_hooks("TimeslowdownHooks", false, "DisplayCommander.HookSuppression"),
      suppress_debug_output_hooks(
          "DebugOutputHooks", true,
          "DisplayCommander.HookSuppression"),  // BLACKLISTED: Debug output can be noisy
      suppress_loadlibrary_hooks("SupressAllHooks", false, "DisplayCommander.HookSuppression"),
      suppress_display_settings_hooks("DisplaySettingsHooks", false, "DisplayCommander.HookSuppression"),
      suppress_windows_message_hooks("WindowsMessageHooks", false, "DisplayCommander.HookSuppression"),
      suppress_opengl_hooks("OpenGLHooks", false, "DisplayCommander.HookSuppression"),
      suppress_nvapi_hooks("NvapiHooks", false, "DisplayCommander.HookSuppression"),
      suppress_process_exit_hooks("ProcessExitHooks", false, "DisplayCommander.HookSuppression"),
      suppress_vulkan_loader_hooks("VulkanLoaderHooks", false, "DisplayCommander.HookSuppression") {
    // Initialize the all_settings_ vector
    all_settings_ = {
        &suppress_dxgi_factory_hooks,
        &suppress_dxgi_swapchain_hooks,
        &suppress_sl_proxy_dxgi_swapchain_hooks,
        &suppress_xinput_hooks,
        &suppress_streamline_hooks,
        &suppress_ngx_hooks,
        &suppress_windows_gaming_input_hooks,
        &suppress_api_hooks,
        &suppress_window_api_hooks,
        &suppress_timeslowdown_hooks,
        &suppress_debug_output_hooks,
        &suppress_loadlibrary_hooks,
        &suppress_display_settings_hooks,
        &suppress_windows_message_hooks,
        &suppress_opengl_hooks,
        &suppress_nvapi_hooks,
        &suppress_process_exit_hooks,
        &suppress_vulkan_loader_hooks,
    };
}

void HookSuppressionSettings::LoadAll() {
    LogInfo("HookSuppressionSettings::LoadAll() - Loading hook suppression settings");

    // Use smart logging to show only changed settings
    ui::new_ui::LoadTabSettingsWithSmartLogging(all_settings_, "Hook Suppression");

    LogInfo("HookSuppressionSettings::LoadAll() - Hook suppression settings loaded");
}

std::vector<SettingBase*> HookSuppressionSettings::GetAllSettings() { return all_settings_; }

}  // namespace settings
