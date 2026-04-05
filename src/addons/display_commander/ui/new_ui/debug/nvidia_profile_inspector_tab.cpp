// Source Code <Display Commander> // follow this order for includes in all files + add this comment at the top
#include "nvidia_profile_inspector_tab.hpp"

#include "../../../features/nvidia_profile_inspector/nvidia_profile_inspector.hpp"
#include "../../ui_colors.hpp"

// Libraries <ReShade> / <imgui>
#include <imgui.h>

// Libraries <standard C++>
#include <memory>
#include <string>

namespace ui::new_ui::debug {

void DrawNvidiaProfileInspectorTab(display_commander::ui::IImGuiWrapper& imgui) {
    imgui.Spacing();
    imgui.TextWrapped(
        "Read-only NVIDIA driver profile (DRS) snapshot for this process: DLSS-SR and DLSS-RR **render preset** "
        "overrides (same keys as NVIDIA Profile Inspector). Data is cached for a few seconds; use Refresh to force a "
        "new query.");
    imgui.Spacing();

    if (imgui.Button("Refresh##nvpi_debug")) {
        display_commander::features::nvidia_profile_inspector::InvalidateDriverDlssRenderPresetCache();
    }
    imgui.SameLine();
    if (imgui.Button("Query now (force)##nvpi_debug")) {
        display_commander::features::nvidia_profile_inspector::InvalidateDriverDlssRenderPresetCache();
        (void)display_commander::features::nvidia_profile_inspector::GetDriverDlssRenderPresetSnapshot(true);
    }

    const std::shared_ptr<const display_commander::features::nvidia_profile_inspector::DriverDlssRenderPresetSnapshot>
        snap = display_commander::features::nvidia_profile_inspector::GetDriverDlssRenderPresetSnapshot(false);

    if (!snap) {
        imgui.TextColored(::ui::colors::TEXT_DIMMED, "No snapshot.");
        return;
    }

    imgui.Separator();
    if (!snap->query_succeeded) {
        imgui.TextColored(ImVec4(1.0f, 0.55f, 0.2f, 1.0f), "Query failed: %s", snap->error_message.c_str());
        if (!snap->current_exe_path_utf8.empty()) {
            imgui.TextWrapped("Exe: %s", snap->current_exe_path_utf8.c_str());
        }
        return;
    }

    if (!snap->current_exe_path_utf8.empty()) {
        imgui.TextWrapped("Exe: %s", snap->current_exe_path_utf8.c_str());
    }
    imgui.Text("DRS profile for exe: %s", snap->has_profile ? "found" : "not found");
    if (snap->has_profile && !snap->profile_name.empty()) {
        imgui.Text("Profile name: %s", snap->profile_name.c_str());
    }

    imgui.Separator();
    imgui.TextUnformatted("DLSS-SR render preset (driver)");
    imgui.BulletText("In profile: %s", snap->sr_defined_in_profile ? "yes" : "no (global default)");
    imgui.BulletText("Value: %s (0x%08x)", snap->sr_display.c_str(), static_cast<unsigned>(snap->sr_value_u32));
    imgui.BulletText("Non-default override: %s", snap->sr_is_non_default_override ? "yes" : "no");
    if (snap->sr_is_non_default_override) {
        imgui.TextColored(ImVec4(1.0f, 0.75f, 0.25f, 1.0f),
                          "Driver profile forces a render preset for Super Resolution (not Off / global default).");
    }

    imgui.Spacing();
    imgui.TextUnformatted("DLSS-RR render preset (driver)");
    imgui.BulletText("In profile: %s", snap->rr_defined_in_profile ? "yes" : "no (global default)");
    imgui.BulletText("Value: %s (0x%08x)", snap->rr_display.c_str(), static_cast<unsigned>(snap->rr_value_u32));
    imgui.BulletText("Non-default override: %s", snap->rr_is_non_default_override ? "yes" : "no");
    if (snap->rr_is_non_default_override) {
        imgui.TextColored(ImVec4(1.0f, 0.75f, 0.25f, 1.0f),
                          "Driver profile forces a render preset for Ray Reconstruction (not Off / global default).");
    }
}

}  // namespace ui::new_ui::debug
