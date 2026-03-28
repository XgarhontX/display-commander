#pragma once

namespace display_commander::game_fixes {

// Per-process game quirks: skip installing hooks that break specific titles.
bool ShouldSkipDisplaySettingsHooksForProcess();

}  // namespace display_commander::game_fixes
