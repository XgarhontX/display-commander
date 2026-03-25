#pragma once

#include "imgui_wrapper_base.hpp"

namespace display_commander::ui {

/// Reference font size (ImGui default) used to derive layout scale from the active font.
inline constexpr float kUiBaseFontSize = 13.0f;
/// Default max content width in logical px at `kUiBaseFontSize` (see `scale_px`).
inline constexpr float kUiMaxContentWidthPx = 800.0f;

inline float get_ui_scale(const IImGuiWrapper& imgui) {
    const float font_size = static_cast<const IImGuiWrapper&>(imgui).GetFontSize();
    if (font_size <= 0.0f) {
        return 1.0f;
    }
    return font_size / kUiBaseFontSize;
}

inline float scale_px(const IImGuiWrapper& imgui, float base_px) {
    return base_px * get_ui_scale(imgui);
}

}  // namespace display_commander::ui
