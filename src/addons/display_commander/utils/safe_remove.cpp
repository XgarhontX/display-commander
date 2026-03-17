// Source Code <Display Commander>
#include "safe_remove.hpp"
#include "general_utils.hpp"
#include "logging.hpp"
#include "version_check.hpp"

// Libraries <standard C++>
#include <algorithm>
#include <filesystem>
#include <set>
#include <string>
#include <vector>

// Libraries <Windows.h>
#include <Windows.h>

namespace display_commander::utils {

bool IsSafeTempSubdirPath(const std::filesystem::path& dir) {
    if (dir.empty()) return false;
    std::filesystem::path parent = dir.parent_path();
    std::wstring pid_part = dir.filename().wstring();
    if (pid_part.empty()) return false;
    for (wchar_t c : pid_part) {
        if (c < L'0' || c > L'9') return false;
    }
    return parent.filename() == L"tmp";
}

namespace {

bool IsAllowedSystemTempSubdir(const std::filesystem::path& path, const wchar_t* subdir_name) {
    wchar_t temp_path_buf[MAX_PATH];
    if (GetTempPathW(static_cast<DWORD>(std::size(temp_path_buf)), temp_path_buf) == 0) {
        return false;
    }
    std::filesystem::path allowed = std::filesystem::path(temp_path_buf).lexically_normal() / subdir_name;
    std::filesystem::path normalized = path.lexically_normal();
    return normalized == allowed;
}

bool IsAllowedStagingPath(const std::filesystem::path& path) {
    std::filesystem::path base = version_check::GetDownloadDirectory();
    if (base.empty()) return false;
    std::filesystem::path allowed = (base / L"Debug" / L"_staging_latest_debug").lexically_normal();
    return path.lexically_normal() == allowed;
}

}  // namespace

bool IsAllowedForRemoveAll(const std::filesystem::path& path) {
    if (path.empty()) return false;
    // Require full path (e.g. "C:\...") to avoid removing based on relative paths.
    if (!path.is_absolute()) {
        return false;
    }
    // Never allow removing the Display Commander AppData root (e.g. ...\AppData\Local\Programs\Display_Commander).
    std::filesystem::path dc_appdata = GetDisplayCommanderAppDataFolder();
    if (!dc_appdata.empty()) {
        if (path.lexically_normal() == dc_appdata.lexically_normal()) {
            return false;
        }
    }
    if (IsSafeTempSubdirPath(path)) return true;
    if (IsAllowedSystemTempSubdir(path, L"dc_reshade_update")) return true;
    if (IsAllowedSystemTempSubdir(path, L"dc_reshade_download")) return true;
    if (IsAllowedStagingPath(path)) return true;
    return false;
}

namespace {

std::set<std::wstring> BuildExtensionSet(std::span<const std::wstring> extensions_to_remove) {
    std::set<std::wstring> out;
    for (const std::wstring& ext : extensions_to_remove) {
        std::wstring e = ext;
        std::transform(e.begin(), e.end(), e.begin(), ::towlower);
        out.insert(std::move(e));
    }
    return out;
}

void RemoveEmptySubdirsRecursive(const std::filesystem::path& dir, std::error_code& ec) {
    std::filesystem::directory_iterator it(dir, std::filesystem::directory_options::skip_permission_denied, ec);
    if (ec) return;
    for (const std::filesystem::directory_entry& entry : it) {
        if (ec) return;
        if (!entry.is_directory(ec)) continue;
        if (ec) return;
        RemoveEmptySubdirsRecursive(entry.path(), ec);
        if (ec) return;
    }
    if (std::filesystem::is_empty(dir, ec) && !ec) {
        std::filesystem::remove(dir, ec);
    }
}

}  // namespace

bool SafeRemoveAll(const std::filesystem::path& path,
                   std::span<const std::wstring> extensions_to_remove,
                   std::error_code& ec) {
    ec.clear();
    if (path.empty() || !path.is_absolute()) {
        LogError("SafeRemoveAll: path must be a full path (e.g. C:\\...), refused: %s", path.string().c_str());
        return false;
    }
    std::filesystem::path dc_appdata = GetDisplayCommanderAppDataFolder();
    if (!dc_appdata.empty() && path.lexically_normal() == dc_appdata.lexically_normal()) {
        LogError("SafeRemoveAll: cannot remove Display Commander AppData root, refused: %s", path.string().c_str());
        return false;
    }
    if (!IsAllowedForRemoveAll(path)) {
        LogError("SafeRemoveAll: path not on whitelist, refused: %s", path.string().c_str());
        return false;
    }
    if (!std::filesystem::exists(path, ec)) {
        return true;
    }
    const std::set<std::wstring> ext_set = BuildExtensionSet(extensions_to_remove);
    std::vector<std::filesystem::path> to_remove;
    std::filesystem::recursive_directory_iterator rec_it(
        path, std::filesystem::directory_options::skip_permission_denied, ec);
    if (ec) return false;
    const auto rec_end = std::filesystem::recursive_directory_iterator();
    for (; rec_it != rec_end; ++rec_it) {
        std::error_code ec_entry;
        if (!rec_it->is_regular_file(ec_entry)) continue;
        if (ec_entry) {
            ec = ec_entry;
            return false;
        }
        std::wstring ext = rec_it->path().extension().wstring();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);
        if (ext_set.count(ext) == 0) continue;
        to_remove.push_back(rec_it->path());
    }
    for (const std::filesystem::path& p : to_remove) {
        std::filesystem::remove(p, ec);
        if (ec) return false;
    }
    RemoveEmptySubdirsRecursive(path, ec);
    return !ec;
}

}  // namespace display_commander::utils
