// Source Code <Display Commander> // follow this order for includes in all files + add this comment at the top
#pragma once

// Source Code <Display Commander>
namespace display_commander::ui {
struct IImGuiWrapper;
}

namespace modules::controller {

/** Full Controller tab: polling rates, XInput widget, remapping. */
void DrawControllerTab(display_commander::ui::IImGuiWrapper& imgui);

}  // namespace modules::controller
