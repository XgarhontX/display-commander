// Source Code <Display Commander> // follow this order for includes in all files + add this comment at the top
// Headers <Display Commander>
#include "performance_overlay_internal.hpp"
#include "nvapi/nvapi_actual_refresh_rate_monitor.hpp"

namespace ui::new_ui {

void DrawRefreshRateFrameTimesGraph(display_commander::ui::IImGuiWrapper& imgui, bool show_tooltips) {
    (void)imgui;
    CALL_GUARD_NO_TS();
    static std::vector<float> frame_times;
    frame_times.clear();
    frame_times.reserve(256);

    display_commander::nvapi::ForEachNvapiActualRefreshRateSample([&](double rate) {
        if (rate > 0.0) {
            frame_times.push_back(static_cast<float>(1000.0 / rate));
        }
    });

    if (frame_times.empty()) {
        if (display_commander::nvapi::IsNvapiActualRefreshRateMonitoringActive()
            && display_commander::nvapi::IsNvapiGetAdaptiveSyncDataFailingRepeatedly()) {
            imgui.TextColored(ui::colors::TEXT_WARNING,
                              "NvAPI_DISP_GetAdaptiveSyncData failing repeatedly — no refresh rate data.");
        }
        return;
    }

    std::reverse(frame_times.begin(), frame_times.end());

    float min_frame_time = *std::ranges::min_element(frame_times);
    float max_frame_time = *std::ranges::max_element(frame_times);
    float avg_frame_time = 0.0f;
    for (float ft : frame_times) {
        avg_frame_time += ft;
    }
    avg_frame_time /= static_cast<float>(frame_times.size());

    float variance = 0.0f;
    for (float ft : frame_times) {
        float diff = ft - avg_frame_time;
        variance += diff * diff;
    }
    variance /= static_cast<float>(frame_times.size());
    float std_deviation = std::sqrt(variance);

    float graph_scale = settings::g_mainTabSettings.overlay_graph_scale.GetValue();
    ImVec2 graph_size = ImVec2(300.0f * graph_scale, 60.0f * graph_scale);
    float scale_min = 0.0f;
    float max_scale = settings::g_mainTabSettings.overlay_graph_max_scale.GetValue();
    float scale_max = avg_frame_time * max_scale;

    float chart_alpha = settings::g_mainTabSettings.overlay_chart_alpha.GetValue();
    ImVec4 bg_color = imgui.GetStyle().Colors[ImGuiCol_FrameBg];
    bg_color.w *= chart_alpha;

    imgui.PushStyleColor(ImGuiCol_FrameBg, bg_color);

    imgui.PlotLines("##RefreshRateFrameTime", frame_times.data(), static_cast<int>(frame_times.size()), 0, nullptr,
                    scale_min, scale_max, graph_size);

    imgui.PopStyleColor();

    bool show_refresh_rate_frame_time_stats = settings::g_mainTabSettings.show_refresh_rate_frame_time_stats.GetValue();
    if (show_refresh_rate_frame_time_stats) {
        imgui.Text("Avg: %.2f ms | Dev: %.2f ms | Min: %.2f ms | Max: %.2f ms", avg_frame_time, std_deviation,
                   min_frame_time, max_frame_time);
    }

    if (imgui.IsItemHovered() && show_tooltips) {
        imgui.SetTooltipEx(
            "Actual refresh rate frame time graph (NvAPI_DISP_GetAdaptiveSyncData) in milliseconds.\n"
            "Lower values = higher refresh rate.\n"
            "Spikes indicate refresh rate variations (VRR, power management, etc.).");
    }
}

}  // namespace ui::new_ui
