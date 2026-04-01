// Source Code <Display Commander> // follow this order for includes in all files + add this comment at the top
#include "audio_module.hpp"

// Source Code <Display Commander>
#include "backend/audio_backend.hpp"
#include "../../globals.hpp"
#include "../../settings/main_tab_settings.hpp"
#include "../../ui/new_ui/main_new_tab.hpp"
#include "../../utils/logging.hpp"

// Libraries <standard C++>
#include <algorithm>
#include <iomanip>
#include <sstream>

namespace modules::audio {
namespace {

void HotkeyMuteToggle() {
    bool new_mute_state = !settings::g_mainTabSettings.audio_mute.GetValue();
    settings::g_mainTabSettings.audio_mute.SetValue(new_mute_state);
    if (::SetMuteForCurrentProcess(new_mute_state)) {
        ::g_muted_applied.store(new_mute_state);
        std::ostringstream oss;
        oss << "Audio " << (new_mute_state ? "muted" : "unmuted") << " via module hotkey";
        LogInfo(oss.str().c_str());
    }
}

void HotkeyVolumeUp() {
    float current_volume = 0.0f;
    if (!::GetVolumeForCurrentProcess(&current_volume)) {
        current_volume = ::s_game_volume_percent.load();
    }

    float step = 0.0f;
    if (current_volume <= 0.0f) {
        step = 1.0f;
    } else {
        step = (std::max)(1.0f, current_volume * 0.20f);
    }

    if (::AdjustVolumeForCurrentProcess(step)) {
        std::ostringstream oss;
        oss << "Volume increased by " << std::fixed << std::setprecision(1) << step << "% via module hotkey";
        LogInfo(oss.str().c_str());
    } else {
        LogWarn("Failed to increase volume via module hotkey");
    }
}

void HotkeyVolumeDown() {
    float current_volume = 0.0f;
    if (!::GetVolumeForCurrentProcess(&current_volume)) {
        current_volume = ::s_game_volume_percent.load();
    }

    if (current_volume <= 0.0f) {
        return;
    }

    const float step = (std::max)(1.0f, current_volume * 0.20f);
    if (::AdjustVolumeForCurrentProcess(-step)) {
        std::ostringstream oss;
        oss << "Volume decreased by " << std::fixed << std::setprecision(1) << step << "% via module hotkey";
        LogInfo(oss.str().c_str());
    } else {
        LogWarn("Failed to decrease volume via module hotkey");
    }
}

}  // namespace

void Initialize(ModuleConfigApi* config_api) {
    (void)config_api;
}

void DrawTab(display_commander::ui::IImGuiWrapper& imgui, reshade::api::effect_runtime* runtime) {
    (void)runtime;
    ui::new_ui::DrawAudioSettings(imgui);
}

void DrawOverlay(display_commander::ui::IImGuiWrapper& imgui) {
    if (settings::g_mainTabSettings.show_overlay_vu_bars.GetValue()) {
        ui::new_ui::DrawOverlayVUBars(imgui, false);
    }
}

void FillHotkeys(std::vector<ModuleHotkeySpec>* hotkeys_out) {
    if (hotkeys_out == nullptr) {
        return;
    }

    hotkeys_out->push_back(ModuleHotkeySpec{
        "mute_unmute", "Mute/Unmute Audio", "ctrl shift m", "Toggle audio mute state", &HotkeyMuteToggle});
    hotkeys_out->push_back(ModuleHotkeySpec{
        "volume_up", "Volume Up", "ctrl shift up", "Increase audio volume (percentage-based, min 1%)",
        &HotkeyVolumeUp});
    hotkeys_out->push_back(ModuleHotkeySpec{
        "volume_down", "Volume Down", "ctrl shift down", "Decrease audio volume (percentage-based, min 1%)",
        &HotkeyVolumeDown});
}

}  // namespace modules::audio
