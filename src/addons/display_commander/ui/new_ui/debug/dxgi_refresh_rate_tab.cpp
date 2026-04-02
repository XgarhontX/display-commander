// Source Code <Display Commander> // follow this order for includes in all files + add this comment at the top
#include "dxgi_refresh_rate_tab.hpp"
#include "../../../settings/advanced_tab_settings.hpp"
#include "../../../latent_sync/refresh_rate_monitor_integration.hpp"
#include "../../../utils/logging.hpp"
#include "../../../utils/timing.hpp"
#include "../settings_wrapper.hpp"
#include "../../ui_colors.hpp"

// Libraries <standard C++>
#include <cstdint>

namespace ui::new_ui::debug {

void DrawDxgiRefreshRateTab(display_commander::ui::IImGuiWrapper& imgui) {
    imgui.Indent();

    if (CheckboxSetting(settings::g_advancedTabSettings.enable_dxgi_refresh_rate_vrr_detection,
                        "DXGI refresh rate / VRR detection", imgui)) {
        LogInfo(
            "DXGI refresh rate / VRR detection setting changed to: %s",
            settings::g_advancedTabSettings.enable_dxgi_refresh_rate_vrr_detection.GetValue() ? "enabled" : "disabled");
    }
    if (imgui.IsItemHovered()) {
        imgui.SetTooltipEx(
            "When enabled, DXGI Present detours signal the refresh rate monitor after each Present.\n"
            "This allows DXGI-based refresh rate and VRR detection (RefreshRateMonitor thread).\n"
            "When disabled, SignalRefreshRateMonitor is not called, reducing per-frame work.\n\n"
            "Default: disabled.");
    }

    imgui.Spacing();
    imgui.TextUnformatted("Debug stats:");
    imgui.Indent();

    const bool thread_running = dxgi::fps_limiter::IsRefreshRateMonitorThreadRunning();
    imgui.Text("Monitor thread: %s", thread_running ? "Running" : "Stopped");
    if (imgui.IsItemHovered()) {
        imgui.SetTooltipEx("RefreshRateMonitor monitoring thread (started with StartRefreshRateMonitoring).");
    }

    const uint64_t signal_count = dxgi::fps_limiter::GetRefreshRateMonitorSignalCount();
    imgui.Text("Signals: %llu", static_cast<unsigned long long>(signal_count));
    if (imgui.IsItemHovered()) {
        imgui.SetTooltipEx(
            "Number of times SignalRefreshRateMonitor was called from DXGI Present detours. "
            "Increments each frame when DXGI refresh rate / VRR detection is enabled. If this stays 0, Present hooks "
            "are not calling the signal.");
    }

    const uint64_t loop_count = dxgi::fps_limiter::GetRefreshRateMonitorLoopCount();
    imgui.Text("Loops: %llu", static_cast<unsigned long long>(loop_count));
    if (imgui.IsItemHovered()) {
        imgui.SetTooltipEx(
            "Number of times the monitor thread loop ran (each wait + process iteration). "
            "Includes timeouts. Resets when monitoring is (re)started. If Loops stays 0, thread may not be running.");
    }

    const bool has_swap = dxgi::fps_limiter::RefreshRateMonitorHasSwapChain();
    imgui.Text("Swap chain: %s", has_swap ? "set" : "not set");
    if (imgui.IsItemHovered()) {
        imgui.SetTooltipEx(
            "Whether a swap chain was stored via SignalPresent (from Present detours). If not set, GetFrameStatistics "
            "is never called.");
    }

    const uint64_t frame_stats_tried = dxgi::fps_limiter::GetRefreshRateMonitorFrameStatsTried();
    const uint64_t frame_stats_ok = dxgi::fps_limiter::GetRefreshRateMonitorFrameStatsOk();
    imgui.Text("GetFrameStatistics: tried %llu, OK %llu", static_cast<unsigned long long>(frame_stats_tried),
               static_cast<unsigned long long>(frame_stats_ok));
    if (imgui.IsItemHovered()) {
        imgui.SetTooltipEx(
            "Times IDXGISwapChain::GetFrameStatistics was called (tried) and returned success (OK). "
            "If tried=0, swap chain was never set when the loop ran. If tried>0 but OK=0, GetFrameStatistics is "
            "failing (e.g. DXGI_ERROR_FRAME_STATISTICS_DISJOINT).");
    }

    const HRESULT last_hr = dxgi::fps_limiter::GetRefreshRateMonitorLastFrameStatisticsHr();
    if (last_hr != 0) {
        imgui.TextColored(::ui::colors::TEXT_ERROR, "Last GetFrameStatistics HRESULT: 0x%08X",
                          static_cast<unsigned>(last_hr));
        if (imgui.IsItemHovered()) {
            imgui.SetTooltipEx(
                "Last failure code from IDXGISwapChain::GetFrameStatistics. "
                "Common: 0x887A0007 = DXGI_ERROR_FRAME_STATISTICS_DISJOINT (reset/transition). Also logged to file "
                "(first + every 100th failure).");
        }
    }

    const uint64_t skipped_no_diff = dxgi::fps_limiter::GetRefreshRateMonitorProcessSkippedNoDiff();
    imgui.Text("Skipped (no diff): %llu", static_cast<unsigned long long>(skipped_no_diff));
    if (imgui.IsItemHovered()) {
        imgui.SetTooltipEx(
            "Times ProcessFrameStatistics got stats but skipped because SyncRefreshCount difference was <= 0 (no new "
            "present since last check).");
    }

    const long long last_ns = dxgi::fps_limiter::GetRefreshRateMonitorLastStatsTimeNs();
    if (last_ns == 0) {
        imgui.Text("Last stats: never");
    } else {
        const LONGLONG now_ns = utils::get_now_ns();
        const double ago_sec = static_cast<double>(now_ns - last_ns) / static_cast<double>(utils::SEC_TO_NS);
        imgui.Text("Last stats: %.2f s ago", ago_sec);
    }
    if (imgui.IsItemHovered()) {
        imgui.SetTooltipEx("Last time GetFrameStatistics was successfully processed (ProcessFrameStatistics).");
    }

    dxgi::fps_limiter::RefreshRateStats stats = dxgi::fps_limiter::GetRefreshRateStats();
    imgui.Text("Status: %s", stats.status.c_str());
    imgui.Text("Samples: %u", stats.sample_count);
    imgui.Text("Current: %.2f Hz | Min: %.2f Hz | Max: %.2f Hz", stats.current_rate, stats.min_rate, stats.max_rate);

    imgui.Unindent();
    imgui.Unindent();
}

}  // namespace ui::new_ui::debug
