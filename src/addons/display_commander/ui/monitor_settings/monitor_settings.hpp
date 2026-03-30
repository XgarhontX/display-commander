#pragma once

#include "../new_ui/settings_wrapper.hpp"

namespace ui::monitor_settings {

// Persisted flags still referenced by main_entry (e.g. reset paths); former Experimental → Monitor Settings UI removed.
extern ui::new_ui::BoolSetting g_setting_auto_apply_resolution;
extern ui::new_ui::BoolSetting g_setting_auto_apply_refresh;
extern ui::new_ui::BoolSetting g_setting_apply_display_settings_at_start;

}  // namespace ui::monitor_settings
