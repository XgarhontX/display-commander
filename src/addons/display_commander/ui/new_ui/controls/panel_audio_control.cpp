// Source Code <Display Commander> // follow this order for includes in all files + add this comment at the top
#include "panels_internal.hpp"

// Source Code <Display Commander>
#include "../../../modules/audio/audio_module.hpp"

namespace ui::new_ui {

void DrawMainTabOptionalPanelAudioControl(display_commander::ui::IImGuiWrapper& imgui) {
    modules::audio::DrawMainTabInline(imgui, nullptr);
}

}  // namespace ui::new_ui
