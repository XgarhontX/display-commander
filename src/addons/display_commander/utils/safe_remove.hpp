#pragma once

// Centralized whitelist for directory removal. Use SafeRemoveAll instead of
// std::filesystem::remove_all to avoid accidentally deleting wrong paths.

// Libraries <standard C++>
#include <filesystem>
#include <span>
#include <string>
#include <system_error>

namespace display_commander::utils {

// Returns true only for a path that is .../tmp/<numeric_pid>. Used to avoid
// ever deleting from Display_Commander root or arbitrary paths.
bool IsSafeTempSubdirPath(const std::filesystem::path& dir);

// Returns true if path is an absolute path (e.g. C:\...), not the Display Commander
// AppData root (e.g. ...\AppData\Local\Programs\Display_Commander), and on the whitelist
// for recursive directory removal:
// - .../tmp/<numeric_pid> (post-ReShade addon temp)
// - <GetTempPath()>/dc_reshade_update
// - <GetTempPath()>/dc_reshade_download
// - <GetDownloadDirectory()>/Debug/_staging_latest_debug
bool IsAllowedForRemoveAll(const std::filesystem::path& path);

// If path is an absolute path and allowed (IsAllowedForRemoveAll), recursively
// removes only files whose extension is in extensions_to_remove (case-insensitive),
// then removes empty directories. Does not remove_all. Returns true if path did
// not exist or removal succeeded; false if not allowed or a remove failed.
bool SafeRemoveAll(const std::filesystem::path& path,
                   std::span<const std::wstring> extensions_to_remove,
                   std::error_code& ec);

}  // namespace display_commander::utils
