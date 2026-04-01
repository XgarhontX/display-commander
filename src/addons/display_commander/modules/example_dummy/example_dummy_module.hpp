// Source Code <Display Commander> // follow this order for includes in all files + add this comment at the top
#pragma once

// Source Code <Display Commander>
#include "modules/module_registry.hpp"

// Libraries <ReShade> / <imgui>
#include <reshade.hpp>

namespace modules::example_dummy {

void Initialize(modules::ModuleConfigApi* config_api);
void Tick();
void DrawTab(display_commander::ui::IImGuiWrapper& imgui, reshade::api::effect_runtime* runtime);
void DrawOverlay(display_commander::ui::IImGuiWrapper& imgui);

}  // namespace modules::example_dummy
