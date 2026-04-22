// Source Code <Display Commander> // follow this order for includes in all files + add this comment at the top
#include "cpu_telemetry.hpp"
#include "../../globals.hpp"
#include "../../settings/main_tab_settings.hpp"

// Libraries <Standard C++>
#include <algorithm>
#include <atomic>
#include <cstdint>

// Windows.h
#include <Windows.h>

namespace display_commander::feature::cpu_telemetry {

namespace {

// Only honor overlay requests for this many frames after the request frame id was set.
constexpr uint64_t kMaxFramesAfterRequest = 100;
// At most one CPU API query per this many game frames.
constexpr uint64_t kMinFramesBetweenQueries = 100;

std::atomic<std::uint32_t> g_cached_process_cpu_percent_x100{0};
std::atomic<std::uint32_t> g_cached_system_cpu_percent_x100{0};
std::atomic<bool> g_cached_process_cpu_valid{false};
std::atomic<bool> g_cached_system_cpu_valid{false};

struct CpuSampleSnapshot {
    std::uint64_t process_kernel_100ns = 0;
    std::uint64_t process_user_100ns = 0;
    std::uint64_t system_idle_100ns = 0;
    std::uint64_t system_kernel_100ns = 0;
    std::uint64_t system_user_100ns = 0;
};

std::uint64_t FileTimeToU64(const FILETIME& ft) {
    ULARGE_INTEGER v{};
    v.LowPart = ft.dwLowDateTime;
    v.HighPart = ft.dwHighDateTime;
    return static_cast<std::uint64_t>(v.QuadPart);
}

bool SampleCpu(CpuSampleSnapshot& out) {
    FILETIME proc_create{};
    FILETIME proc_exit{};
    FILETIME proc_kernel{};
    FILETIME proc_user{};
    if (GetProcessTimes(GetCurrentProcess(), &proc_create, &proc_exit, &proc_kernel, &proc_user) == 0) {
        return false;
    }

    FILETIME sys_idle{};
    FILETIME sys_kernel{};
    FILETIME sys_user{};
    if (GetSystemTimes(&sys_idle, &sys_kernel, &sys_user) == 0) {
        return false;
    }

    out.process_kernel_100ns = FileTimeToU64(proc_kernel);
    out.process_user_100ns = FileTimeToU64(proc_user);
    out.system_idle_100ns = FileTimeToU64(sys_idle);
    out.system_kernel_100ns = FileTimeToU64(sys_kernel);
    out.system_user_100ns = FileTimeToU64(sys_user);
    return true;
}

bool QueryCpuLoadPercent(double& out_process_percent, double& out_system_percent) {
    out_process_percent = 0.0;
    out_system_percent = 0.0;

    CpuSampleSnapshot current{};
    if (!SampleCpu(current)) {
        return false;
    }

    static bool s_has_prev = false;
    static CpuSampleSnapshot s_prev{};
    if (!s_has_prev) {
        s_prev = current;
        s_has_prev = true;
        return false;
    }

    const std::uint64_t process_total_prev = s_prev.process_kernel_100ns + s_prev.process_user_100ns;
    const std::uint64_t process_total_cur = current.process_kernel_100ns + current.process_user_100ns;
    const std::uint64_t system_total_prev = s_prev.system_kernel_100ns + s_prev.system_user_100ns;
    const std::uint64_t system_total_cur = current.system_kernel_100ns + current.system_user_100ns;

    const std::uint64_t process_delta =
        (process_total_cur >= process_total_prev) ? (process_total_cur - process_total_prev) : 0;
    const std::uint64_t system_total_delta =
        (system_total_cur >= system_total_prev) ? (system_total_cur - system_total_prev) : 0;
    const std::uint64_t system_idle_delta =
        (current.system_idle_100ns >= s_prev.system_idle_100ns) ? (current.system_idle_100ns - s_prev.system_idle_100ns)
                                                                 : 0;

    s_prev = current;

    if (system_total_delta == 0) {
        return false;
    }

    const std::uint64_t system_busy_delta = (system_total_delta > system_idle_delta) ? (system_total_delta - system_idle_delta) : 0;

    out_process_percent = (static_cast<double>(process_delta) * 100.0) / static_cast<double>(system_total_delta);
    out_system_percent = (static_cast<double>(system_busy_delta) * 100.0) / static_cast<double>(system_total_delta);

    out_process_percent = (std::clamp)(out_process_percent, 0.0, 100.0);
    out_system_percent = (std::clamp)(out_system_percent, 0.0, 100.0);
    return true;
}

}  // namespace

void RequestProcessCpuLoadFromOverlay(bool enabled) {
    if (!enabled) {
        return;
    }
    g_cpu_telemetry_request_frame_id.store(g_global_frame_id.load(std::memory_order_relaxed), std::memory_order_relaxed);
}

void RequestSystemCpuLoadFromOverlay(bool enabled) {
    if (!enabled) {
        return;
    }
    g_cpu_telemetry_request_frame_id.store(g_global_frame_id.load(std::memory_order_relaxed), std::memory_order_relaxed);
}

void ProcessCpuLoadRequestsInContinuousMonitoring() {
    const bool show_process = settings::g_mainTabSettings.show_overlay_cpu_process_load.GetValue();
    const bool show_system = settings::g_mainTabSettings.show_overlay_cpu_system_load.GetValue();
    if (!show_process && !show_system) {
        g_cpu_telemetry_request_frame_id.store(0, std::memory_order_relaxed);
        return;
    }

    const uint64_t req = g_cpu_telemetry_request_frame_id.load(std::memory_order_acquire);
    if (req == 0) {
        return;
    }

    const uint64_t cur = g_global_frame_id.load(std::memory_order_acquire);
    if (cur < req) {
        return;
    }
    if (cur - req > kMaxFramesAfterRequest) {
        return;
    }

    const uint64_t last_q = g_cpu_telemetry_last_query_frame_id.load(std::memory_order_relaxed);
    if (last_q != 0 && cur - last_q < kMinFramesBetweenQueries) {
        return;
    }

    double process_pct = 0.0;
    double system_pct = 0.0;
    if (QueryCpuLoadPercent(process_pct, system_pct)) {
        if (show_process) {
            g_cached_process_cpu_percent_x100.store(
                static_cast<std::uint32_t>((std::clamp)(process_pct, 0.0, 100.0) * 100.0 + 0.5),
                std::memory_order_relaxed);
            g_cached_process_cpu_valid.store(true, std::memory_order_relaxed);
        }
        if (show_system) {
            g_cached_system_cpu_percent_x100.store(
                static_cast<std::uint32_t>((std::clamp)(system_pct, 0.0, 100.0) * 100.0 + 0.5),
                std::memory_order_relaxed);
            g_cached_system_cpu_valid.store(true, std::memory_order_relaxed);
        }
    }

    // On failure, keep previous cache; still advance last_query_frame to avoid hammering APIs.
    g_cpu_telemetry_last_query_frame_id.store(cur, std::memory_order_relaxed);
}

bool GetCachedProcessCpuLoadPercent(double& out_percent) {
    if (!g_cached_process_cpu_valid.load(std::memory_order_relaxed)) {
        return false;
    }
    out_percent = static_cast<double>(g_cached_process_cpu_percent_x100.load(std::memory_order_relaxed)) / 100.0;
    return true;
}

bool GetCachedSystemCpuLoadPercent(double& out_percent) {
    if (!g_cached_system_cpu_valid.load(std::memory_order_relaxed)) {
        return false;
    }
    out_percent = static_cast<double>(g_cached_system_cpu_percent_x100.load(std::memory_order_relaxed)) / 100.0;
    return true;
}

}  // namespace display_commander::feature::cpu_telemetry
