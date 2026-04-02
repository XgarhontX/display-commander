#pragma once

#include <string>

namespace display_commander::config {

// Path to global_overrides.toml in Display Commander folder (Local App Data). Shared across all games.
// Keys in [DisplayCommander] match config keys they override; values here override game config even when it exists.
std::string GetGlobalOverridesFilePath();

// Load overrides from file into cache. Ensure directory exists. Returns true if file was read (or created empty).
bool LoadGlobalOverridesFile();

// Save current cache to global_overrides.toml. Returns true on success.
bool SaveGlobalOverridesFile();

// Get value from cache (loads file on first use). Returns true if key exists in override file.
bool GetGlobalOverrideValue(const char* key, std::string& value);

// Set value in cache and save to file. Use for keys that should override the same key in game config (e.g. SuppressWgiEnabled).
void SetGlobalOverrideValue(const char* key, const std::string& value);

}  // namespace display_commander::config
