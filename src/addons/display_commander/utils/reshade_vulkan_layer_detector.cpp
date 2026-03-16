// Source Code <Display Commander>
#include "reshade_vulkan_layer_detector.hpp"

// Libraries <standard C++>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

// Libraries <Windows.h>
#include <Windows.h>

namespace {

constexpr const char* RESHADE_APPS_INI_PATH = "C:\\ProgramData\\ReShade\\ReShadeApps.ini";
constexpr const char APPS_KEY[] = "Apps";

std::string Trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return {};
    auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

// Split "Apps=path1,path2,path3" into ["path1","path2","path3"] after trimming.
std::vector<std::string> ParseAppsValue(const std::string& value) {
    std::vector<std::string> paths;
    std::istringstream ss(value);
    std::string part;
    while (std::getline(ss, part, ',')) {
        std::string p = Trim(part);
        if (!p.empty()) paths.push_back(p);
    }
    return paths;
}

bool ParseReShadeAppsIni(std::vector<std::string>& out_app_paths) {
    std::ifstream f(RESHADE_APPS_INI_PATH);
    if (!f) return false;
    std::string line;
    while (std::getline(f, line)) {
        std::string trimmed = Trim(line);
        if (trimmed.empty() || trimmed[0] == ';' || trimmed[0] == '#') continue;
        size_t eq = trimmed.find('=');
        if (eq == std::string::npos) continue;
        std::string key = Trim(trimmed.substr(0, eq));
        std::string val = Trim(trimmed.substr(eq + 1));
        if (key != APPS_KEY) continue;
        out_app_paths = ParseAppsValue(val);
        return true;
    }
    return false;
}

}  // namespace

bool IsReShadeRegisteredAsVulkanLayerForCurrentExe() {
    wchar_t exe_buf[MAX_PATH];
    if (GetModuleFileNameW(nullptr, exe_buf, MAX_PATH) == 0) return false;
    std::filesystem::path current_exe(exe_buf);
    std::error_code ec;
    current_exe = std::filesystem::canonical(current_exe, ec);
    if (ec) current_exe = std::filesystem::path(exe_buf);

    std::vector<std::string> app_paths;
    if (!ParseReShadeAppsIni(app_paths)) return false;

    for (const std::string& ap : app_paths) {
        std::filesystem::path listed(ap);
        if (listed.empty()) continue;
        if (!std::filesystem::exists(listed, ec)) continue;
        if (std::filesystem::equivalent(current_exe, listed, ec)) return true;
    }
    return false;
}
