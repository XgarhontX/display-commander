#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace display_commander::utils {

// Returns the directory that should contain Reshade64.dll / Reshade32.dll.
// Behavior: if ReShade is not loaded yet, try local (game_directory) first, then global
// (%LocalAppData%\Programs\Display_Commander\Reshade). Empty path when load is disabled ("no").
std::filesystem::path GetReshadeDirectoryForLoading(const std::filesystem::path& game_directory);

// Overload for UI/callers without game context: uses empty game_directory (no Local entry).
std::filesystem::path GetReshadeDirectoryForLoading();

// Global ReShade base path (%localappdata%\Programs\Display_Commander\Reshade). For UI and load path selection.
std::filesystem::path GetGlobalReshadeDirectory();

// Version string of Reshade64.dll in the given directory, or empty if not found. For UI (loaded version, etc.).
std::string GetReshadeVersionInDirectory(const std::filesystem::path& dir);

// Version string of Reshade64.dll in the global base folder, or empty if not found.
std::string GetGlobalReshadeVersion();

// Config get/set. Value: "global", "local", "latest", "X.Y.Z", or "no".
std::string GetReshadeSelectedVersionFromConfig();
void SetReshadeSelectedVersionInConfig(const std::string& version);

// Supported ReShade versions for dropdown (all known: fallback + GitHub).
const char* const* GetReshadeVersionList(size_t* out_count);

// Delete local ReShade DLLs (Reshade64.dll, Reshade32.dll) from the given directory (e.g. game exe folder).
// Safe because we never load the game-folder copy directly; we copy to temp then load. Returns true on success.
bool DeleteLocalReshadeFromDirectory(const std::filesystem::path& dir, std::string* out_error = nullptr);

}  // namespace display_commander::utils
