// Source Code <Display Commander> // follow this order for includes in all files + add this comment at the top
#include "controller_module.hpp"
#include "controller_tab.hpp"

// Source Code <Display Commander>
#include "../../utils/logging.hpp"
#include "input_remapping.hpp"
#include "remapping_widget.hpp"
#include "windows_gaming_input_hooks.hpp"
#include "xinput_hooks.hpp"
#include "xinput_widget.hpp"

// Libraries <ReShade> / <imgui>
#include <reshade.hpp>

// Libraries <standard C++>
#include <cwchar>

namespace modules::controller {

namespace {
void UninstallWindowsGamingInputHooksForController() {
    display_commanderhooks::UninstallWindowsGamingInputHooks();
    LogInfo("[ControllerModule] Windows.Gaming.Input hooks uninstalled");
}
}  // namespace

void Initialize(ModuleConfigApi* config_api) {
    (void)config_api;
    display_commander::widgets::xinput_widget::InitializeXInputWidget();
    display_commander::widgets::remapping_widget::InitializeRemappingWidget();
    display_commander::input_remapping::initialize_input_remapping();
    LogInfo("[ControllerModule] Input remapping initialized");
}

void OnEnabled() {
    if (display_commanderhooks::InstallXInputHooks(nullptr)) {
        LogInfo("[ControllerModule] XInput hooks installed from OnEnabled");
    }
}

void OnDisabled() { UninstallWindowsGamingInputHooksForController(); }

void OnUninstallApiHooks() { UninstallWindowsGamingInputHooksForController(); }

void RetryInstallXInputHooksIfEnabled() {
    if (!modules::IsModuleEnabled("controller")) {
        return;
    }
    if (display_commanderhooks::InstallXInputHooks(nullptr)) {
        LogInfo("[ControllerModule] XInput hooks retry installed");
    }
}

void OnReshadePresentBefore() { display_commander::widgets::xinput_widget::CheckAndHandleScreenshot(); }

void OnLibraryLoaded(HMODULE h_module, const wchar_t* module_path_lower) {
    if (!modules::IsModuleEnabled("controller") || h_module == nullptr || module_path_lower == nullptr) {
        return;
    }
    if (std::wcsstr(module_path_lower, L"xinput") != nullptr) {
        if (display_commanderhooks::InstallXInputHooks(h_module)) {
            LogInfo("[ControllerModule] XInput hooks installed for loaded module");
        }
    }
    if (std::wcsstr(module_path_lower, L"windows.gaming.input.dll") != nullptr) {
        if (display_commanderhooks::InstallWindowsGamingInputHooks(h_module)) {
            LogInfo("[ControllerModule] Windows.Gaming.Input hooks installed for loaded module");
        }
    }
}

void DrawTab(display_commander::ui::IImGuiWrapper& imgui, reshade::api::effect_runtime* runtime) {
    (void)runtime;
    DrawControllerTab(imgui);
}

}  // namespace modules::controller
