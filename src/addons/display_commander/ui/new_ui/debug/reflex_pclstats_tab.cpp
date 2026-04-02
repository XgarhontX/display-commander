// Source Code <Display Commander> // follow this order for includes in all files + add this comment at the top
#include "reflex_pclstats_tab.hpp"
#include "../../../globals.hpp"
#include "../../../latency/reflex_provider.hpp"
#include "../../../settings/main_tab_settings.hpp"
#include "../../ui_colors.hpp"

// Libraries <ReShade> / <imgui>
#include <imgui.h>

// Libraries <standard C++>
#include <cinttypes>
#include <cstddef>
#include <cstdio>
#include <vector>

// Libraries <Windows.h> — before other Windows headers
#include <Windows.h>

namespace ui::new_ui::debug {

namespace {

const char* PclStatsMarkerSlotLabel(std::size_t i) {
    static const char* const kKnown[] = {
        "SIMULATION_START",
        "SIMULATION_END",
        "RENDERSUBMIT_START",
        "RENDERSUBMIT_END",
        "PRESENT_START",
        "PRESENT_END",
        "INPUT_SAMPLE (deprecated)",
        "TRIGGER_FLASH",
        "PC_LATENCY_PING",
        "OUT_OF_BAND_RENDERSUBMIT_START",
        "OUT_OF_BAND_RENDERSUBMIT_END",
        "OUT_OF_BAND_PRESENT_START",
        "OUT_OF_BAND_PRESENT_END",
        "CONTROLLER_INPUT_SAMPLE",
        "DELTA_T_CALCULATION",
        "LATE_WARP_PRESENT_START",
        "LATE_WARP_PRESENT_END",
        "CAMERA_CONSTRUCTED",
        "LATE_WARP_SUBMIT_START",
        "LATE_WARP_SUBMIT_END",
    };
    constexpr std::size_t kKnownCount = sizeof(kKnown) / sizeof(kKnown[0]);
    if (i < kKnownCount) {
        return kKnown[i];
    }
    if (i == kPclStatsEtwMarkerSlotCount - 1) {
        return "clamped (marker id >= slot count)";
    }
    return "reserved";
}

}  // namespace

void DrawReflexPclstatsTab(display_commander::ui::IImGuiWrapper& imgui) {
    imgui.TextColored(::ui::colors::TEXT_DIMMED,
                      "Build with DEBUG_TABS / bd.ps1 -DebugTabs. PCLStats ETW counts include ping + injected markers "
                      "when \"PCL stats for injected reflex\" is on.");

    imgui.Spacing();
    imgui.Separator();
    imgui.Spacing();

    imgui.TextColored(ImVec4{0.85f, 0.85f, 0.85f, 1.0f}, "Reflex provider");
    imgui.Indent();
    if (!g_reflexProvider) {
        imgui.TextColored(::ui::colors::TEXT_DIMMED, "g_reflexProvider: (null)");
    } else {
        imgui.Text("Initialized: %s", g_reflexProvider->IsInitialized() ? "yes" : "no");
        ReflexProvider::NvapiLatencyMetrics metrics{};
        if (g_reflexProvider->GetLatencyMetrics(metrics)) {
            imgui.Text("NVAPI latency (rolling): PC %.3f ms, GPU frame %.3f ms, FrameID %" PRIu64,
                       metrics.pc_latency_ms, metrics.gpu_frame_time_ms,
                       static_cast<unsigned long long>(metrics.frame_id));
        } else {
            imgui.TextColored(::ui::colors::TEXT_DIMMED, "NVAPI GetLatency metrics: (unavailable)");
        }
    }
    imgui.Unindent();

    imgui.Spacing();
    imgui.Separator();
    imgui.Spacing();

    imgui.TextColored(ImVec4{0.85f, 0.85f, 0.85f, 1.0f}, "Injected Reflex (NVAPI path counters)");
    imgui.Indent();
    imgui.Text("Sleep calls: %" PRIu32, g_reflex_sleep_count.load());
    imgui.Text("ApplySleepMode calls: %" PRIu32, g_reflex_apply_sleep_mode_count.load());
    imgui.Text("SIMULATION_START: %" PRIu32, g_reflex_marker_simulation_start_count.load());
    imgui.Text("SIMULATION_END: %" PRIu32, g_reflex_marker_simulation_end_count.load());
    imgui.Text("RENDERSUBMIT_START: %" PRIu32, g_reflex_marker_rendersubmit_start_count.load());
    imgui.Text("RENDERSUBMIT_END: %" PRIu32, g_reflex_marker_rendersubmit_end_count.load());
    imgui.Text("PRESENT_START: %" PRIu32, g_reflex_marker_present_start_count.load());
    imgui.Text("PRESENT_END: %" PRIu32, g_reflex_marker_present_end_count.load());
    imgui.Text("INPUT_SAMPLE: %" PRIu32, g_reflex_marker_input_sample_count.load());
    imgui.Unindent();

    imgui.Spacing();
    imgui.Separator();
    imgui.Spacing();

    imgui.TextColored(ImVec4{0.85f, 0.85f, 0.85f, 1.0f}, "PCLStats provider");
    imgui.Indent();
    const bool pcl_user = settings::g_mainTabSettings.pcl_stats_enabled.GetValue();
    imgui.Text("Setting \"PCL stats for injected reflex\": %s", pcl_user ? "on" : "off");
    imgui.Text("PCLStats initialized: %s", ReflexProvider::IsPCLStatsInitialized() ? "yes" : "no");
    imgui.Text("PCLSTATS_INIT success count (session): %" PRIu64,
               static_cast<unsigned long long>(g_pclstats_init_success_count.load()));
    const LONGLONG last_init = g_pclstats_last_init_time_ns.load();
    if (last_init != 0) {
        imgui.Text("Last PCLSTATS_INIT time (ns since epoch): %" PRId64, static_cast<int64_t>(last_init));
    } else {
        imgui.TextColored(::ui::colors::TEXT_DIMMED, "Last PCLSTATS_INIT: (never)");
    }
    imgui.Text("g_pclstats_frame_id: %" PRIu64,
               static_cast<unsigned long long>(g_pclstats_frame_id.load(std::memory_order_relaxed)));
    imgui.Unindent();

    imgui.Spacing();
    imgui.Separator();
    imgui.Spacing();

    imgui.TextColored(ImVec4{0.85f, 0.85f, 0.85f, 1.0f}, "PCLStats ETW (PCLStatsEvent) emit counts");
    imgui.Indent();
    const uint64_t etw_total = g_pclstats_etw_total_count.load(std::memory_order_relaxed);
    imgui.Text("Total emits: %" PRIu64, static_cast<unsigned long long>(etw_total));
    if (etw_total == 0) {
        imgui.TextColored(::ui::colors::TEXT_DIMMED, "No ETW marker emits recorded yet.");
    } else if (imgui.BeginTable("pclstats_etw_counts", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        imgui.TableSetupColumn("Marker id");
        imgui.TableSetupColumn("Name");
        imgui.TableSetupColumn("Count");
        imgui.TableHeadersRow();
        for (std::size_t i = 0; i < kPclStatsEtwMarkerSlotCount; ++i) {
            const uint64_t c = g_pclstats_etw_by_marker[i].load(std::memory_order_relaxed);
            if (c == 0) {
                continue;
            }
            imgui.TableNextRow();
            imgui.TableNextColumn();
            imgui.Text("%zu", i);
            imgui.TableNextColumn();
            imgui.TextUnformatted(PclStatsMarkerSlotLabel(i));
            imgui.TableNextColumn();
            imgui.Text("%" PRIu64, static_cast<unsigned long long>(c));
        }
        imgui.EndTable();
    }
    imgui.Unindent();

    imgui.Spacing();
    imgui.Separator();
    imgui.Spacing();

    imgui.TextColored(ImVec4{0.85f, 0.85f, 0.85f, 1.0f}, "Recent NVAPI frame reports (up to 10)");
    imgui.Indent();
    static std::vector<ReflexProvider::NvapiLatencyFrame> s_frames;
    if (imgui.Button("Refresh latency frames") && g_reflexProvider) {
        (void)g_reflexProvider->GetRecentLatencyFrames(s_frames, 10);
    }
    if (g_reflexProvider && !s_frames.empty()) {
        if (imgui.BeginTable("reflex_recent_frames", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            imgui.TableSetupColumn("FrameID");
            imgui.TableSetupColumn("Sim start (ns)");
            imgui.TableSetupColumn("Present end (ns)");
            imgui.TableSetupColumn("GPU end (ns)");
            imgui.TableHeadersRow();
            for (const auto& fr : s_frames) {
                imgui.TableNextRow();
                imgui.TableNextColumn();
                imgui.Text("%" PRIu64, static_cast<unsigned long long>(fr.frame_id));
                imgui.TableNextColumn();
                imgui.Text("%" PRIu64, static_cast<unsigned long long>(fr.sim_start_time_ns));
                imgui.TableNextColumn();
                imgui.Text("%" PRIu64, static_cast<unsigned long long>(fr.present_end_time_ns));
                imgui.TableNextColumn();
                imgui.Text("%" PRIu64, static_cast<unsigned long long>(fr.gpu_render_end_time_ns));
            }
            imgui.EndTable();
        }
    } else {
        imgui.TextColored(::ui::colors::TEXT_DIMMED, "Press Refresh (requires initialized Reflex provider).");
    }
    imgui.Unindent();
}

}  // namespace ui::new_ui::debug
