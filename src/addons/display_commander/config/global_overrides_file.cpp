// Source Code <Display Commander> // follow this order for includes in all files + add this comment at the top
#include "global_overrides_file.hpp"
#include "toml_line_parser.hpp"
#include "../utils/general_utils.hpp"
#include "../utils/logging.hpp"

#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>

namespace display_commander::config {

namespace {

std::map<std::string, std::string> g_global_overrides_cache;
bool g_global_overrides_loaded = false;

const char* const GLOBAL_OVERRIDES_TEMPLATE = R"(# Display Commander — Global overrides
# Location: %LocalAppData%\Programs\Display_Commander\global_overrides.toml
#
# Same format as default_settings.toml, but values here override the game's DisplayCommander.toml
# even when the game config already has the key. Use keys that match what they override.
#
# [DisplayCommander]
# auto_reshade_config_backup = true
# SuppressWgiEnabled = false
)";

// Migrate from legacy global_settings.toml if present (one-time: read old keys, write new keys to overrides file).
void MigrateFromGlobalSettingsIfNeeded(const std::string& overrides_path) {
    std::filesystem::path dir = std::filesystem::path(overrides_path).parent_path();
    if (dir.empty()) return;
    std::string legacy_path = (dir / "global_settings.toml").string();
    if (!std::filesystem::exists(legacy_path)) return;

    std::ifstream file(legacy_path);
    if (!file.is_open()) return;

    std::string line;
    bool in_display_commander = false;
    std::string legacy_auto_backup;
    std::string legacy_suppress_wgi;

    while (std::getline(file, line)) {
        size_t start = line.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) continue;
        line = line.substr(start);
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        if (line.empty() || line[0] == '#') continue;
        if (line.front() == '[' && line.back() == ']') {
            std::string sec = line.substr(1, line.size() - 2);
            in_display_commander = (sec == "DisplayCommander");
            continue;
        }
        if (!in_display_commander) continue;
        std::string k;
        std::string v;
        if (ParseTomlLine(line, k, v)) {
            if (k == "AutoEnableReshadeConfigBackup") legacy_auto_backup = v;
            if (k == "SuppressWgiGlobally") legacy_suppress_wgi = v;
        }
    }

    if (legacy_auto_backup.empty() && legacy_suppress_wgi.empty()) return;

    std::map<std::string, std::string> current;
    if (std::filesystem::exists(overrides_path)) {
        std::ifstream ov(overrides_path);
        if (ov.is_open()) {
            std::string ln;
            bool in_dc = false;
            while (std::getline(ov, ln)) {
                size_t st = ln.find_first_not_of(" \t\r\n");
                if (st == std::string::npos) continue;
                ln = ln.substr(st);
                ln.erase(ln.find_last_not_of(" \t\r\n") + 1);
                if (ln.empty() || ln[0] == '#') continue;
                if (ln.front() == '[' && ln.back() == ']') {
                    in_dc = (ln.substr(1, ln.size() - 2) == "DisplayCommander");
                    continue;
                }
                if (!in_dc) continue;
                std::string kk, vv;
                if (ParseTomlLine(ln, kk, vv)) current[kk] = vv;
            }
        }
    }

    bool changed = false;
    if (!legacy_auto_backup.empty() && current.find("auto_reshade_config_backup") == current.end()) {
        current["auto_reshade_config_backup"] = legacy_auto_backup;
        changed = true;
    }
    if (!legacy_suppress_wgi.empty() && current.find("SuppressWgiEnabled") == current.end()) {
        current["SuppressWgiEnabled"] = legacy_suppress_wgi;
        changed = true;
    }
    if (!changed) return;

    std::ofstream out(overrides_path);
    if (!out.is_open()) return;
    out << "# Display Commander — Global overrides\n";
    out << "# Keys match Display Commander config; values override game config.\n\n";
    out << "[DisplayCommander]\n";
    for (const auto& kv : current) {
        out << kv.first << " = " << kv.second << "\n";
    }
    LogInfo("Global overrides: migrated from global_settings.toml to global_overrides.toml");
}

bool EnsureGlobalOverridesFileExists(const std::string& path) {
    if (path.empty()) return false;
    std::filesystem::path dir = std::filesystem::path(path).parent_path();
    if (!dir.empty() && !std::filesystem::exists(dir)) {
        std::error_code ec;
        if (!std::filesystem::create_directories(dir, ec)) {
            LogError("Global overrides file: failed to create directory %s: %s", dir.string().c_str(),
                     ec.message().c_str());
            return false;
        }
    }
    if (std::filesystem::exists(path)) return true;
    std::ofstream file(path);
    if (!file.is_open()) {
        LogError("Global overrides file: failed to create %s", path.c_str());
        return false;
    }
    file << GLOBAL_OVERRIDES_TEMPLATE;
    file.close();
    LogInfo("Global overrides file: created template at %s", path.c_str());
    return true;
}

}  // namespace

std::string GetGlobalOverridesFilePath() {
    std::filesystem::path dir = GetDisplayCommanderAppDataFolder();
    if (dir.empty()) return {};
    return (dir / "global_overrides.toml").string();
}

bool LoadGlobalOverridesFile() {
    std::string path = GetGlobalOverridesFilePath();
    if (path.empty()) return false;

    if (!EnsureGlobalOverridesFileExists(path)) return false;

    MigrateFromGlobalSettingsIfNeeded(path);

    g_global_overrides_cache.clear();
    std::ifstream file(path);
    if (!file.is_open()) {
        g_global_overrides_loaded = true;
        return true;
    }

    std::string line;
    bool in_display_commander = false;
    while (std::getline(file, line)) {
        size_t start = line.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) continue;
        line = line.substr(start);
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        if (line.empty() || line[0] == '#') continue;
        if (line.front() == '[' && line.back() == ']') {
            std::string sec = line.substr(1, line.size() - 2);
            in_display_commander = (sec == "DisplayCommander");
            continue;
        }
        if (!in_display_commander) continue;
        std::string k;
        std::string v;
        if (ParseTomlLine(line, k, v)) {
            g_global_overrides_cache[k] = v;
        }
    }
    g_global_overrides_loaded = true;
    return true;
}

bool SaveGlobalOverridesFile() {
    std::string path = GetGlobalOverridesFilePath();
    if (path.empty()) return false;
    if (!EnsureGlobalOverridesFileExists(path)) return false;

    std::ofstream file(path);
    if (!file.is_open()) {
        LogError("Global overrides file: failed to save %s", path.c_str());
        return false;
    }
    file << "# Display Commander — Global overrides\n";
    file << "# Keys match Display Commander config; values override game config.\n\n";
    file << "[DisplayCommander]\n";
    for (const auto& kv : g_global_overrides_cache) {
        file << kv.first << " = " << kv.second << "\n";
    }
    return true;
}

bool GetGlobalOverrideValue(const char* key, std::string& value) {
    if (key == nullptr) return false;
    if (!g_global_overrides_loaded && !LoadGlobalOverridesFile()) return false;
    auto it = g_global_overrides_cache.find(key);
    if (it == g_global_overrides_cache.end()) return false;
    value = it->second;
    return true;
}

void SetGlobalOverrideValue(const char* key, const std::string& value) {
    if (key == nullptr) return;
    if (!g_global_overrides_loaded) (void)LoadGlobalOverridesFile();
    g_global_overrides_cache[key] = value;
    (void)SaveGlobalOverridesFile();
}

}  // namespace display_commander::config
