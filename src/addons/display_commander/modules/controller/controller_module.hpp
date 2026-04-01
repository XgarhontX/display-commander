// Source Code <Display Commander> // follow this order for includes in all files + add this comment at the top
#pragma once

// Source Code <Display Commander>
#include "../module_registry.hpp"

namespace modules::controller {

void Initialize(ModuleConfigApi* config_api);
void OnEnabled();
void OnDisabled();
/** Uninstall module-owned hooks before global API teardown (`NotifyModulesUninstallApiHooks`). */
void OnUninstallApiHooks();
/** Re-scan for already-loaded XInput DLLs (e.g. after first HWND / present). No-op if the Controller module is disabled. */
void RetryInstallXInputHooksIfEnabled();
void OnLibraryLoaded(HMODULE h_module, const wchar_t* module_path_lower);
/** ReShade present-before path (module registry); runs only when Controller is enabled. */
void OnReshadePresentBefore();
void DrawTab(display_commander::ui::IImGuiWrapper& imgui, reshade::api::effect_runtime* runtime);

}  // namespace modules::controller
