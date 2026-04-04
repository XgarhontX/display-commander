// Source Code <Display Commander> // follow this order for includes in all files + add this comment at the top
#include "reshade_addons_module.hpp"

// Source Code <Display Commander>
#include "../../ui/new_ui/addons_tab.hpp"

// Libraries <ReShade> / <imgui>
#include <reshade.hpp>

namespace modules::reshade_addons {

void Initialize(ModuleConfigApi* config_api) {
    (void)config_api;
    ui::new_ui::InitAddonsTab();
}

void DrawTab(display_commander::ui::IImGuiWrapper& imgui, reshade::api::effect_runtime* runtime) {
    (void)runtime;
    ui::new_ui::DrawAddonsTab(imgui);
}

}  // namespace modules::reshade_addons
