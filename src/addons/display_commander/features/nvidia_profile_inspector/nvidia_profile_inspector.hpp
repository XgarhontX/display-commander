#pragma once

// Read-only NVIDIA driver profile (DRS) queries for DLSS SR/RR render preset overrides.
// No full Profile Inspector UI — logic lives here for Debug tab, DLSS Control panel, and overlay.

#include <cstdint>
#include <memory>
#include <string>

namespace display_commander::features::nvidia_profile_inspector {

struct DriverDlssRenderPresetSnapshot {
    bool query_succeeded = false;
    std::string error_message;
    std::string current_exe_path_utf8;
    bool has_profile = false;
    std::string profile_name;
    bool sr_defined_in_profile = false;
    bool rr_defined_in_profile = false;
    std::uint32_t sr_value_u32 = 0;
    std::uint32_t rr_value_u32 = 0;
    std::string sr_display;
    std::string rr_display;
    bool sr_is_non_default_override = false;
    bool rr_is_non_default_override = false;
};

// Cached snapshot (default ~3 s TTL). Safe from UI threads; uses SRWLOCK for refresh + atomic shared_ptr.
std::shared_ptr<const DriverDlssRenderPresetSnapshot> GetDriverDlssRenderPresetSnapshot(bool force_refresh = false);

void InvalidateDriverDlssRenderPresetCache();

}  // namespace display_commander::features::nvidia_profile_inspector
