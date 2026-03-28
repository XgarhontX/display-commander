// Source Code <Display Commander> // follow this order for includes in all files + add this comment at the top
#include "game_exe_hook_skip.hpp"

// Libraries <standard C++>
#include <algorithm>
#include <filesystem>
#include <string>

// Libraries <Windows.h> — before other Windows headers
#include <Windows.h>

namespace display_commander::game_fixes {

bool ShouldSkipDisplaySettingsHooksForProcess() {
    wchar_t exe_path[MAX_PATH] = {};
    if (GetModuleFileNameW(nullptr, exe_path, MAX_PATH) == 0) {
        return false;
    }
    std::wstring exe_name = std::filesystem::path(exe_path).filename().wstring();
    std::transform(exe_name.begin(), exe_name.end(), exe_name.begin(), ::towlower);
    return exe_name == L"blackops3.exe";
}

}  // namespace display_commander::game_fixes
