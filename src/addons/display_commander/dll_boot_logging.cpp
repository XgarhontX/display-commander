// Source Code <Display Commander> // follow this order for includes in all files + add this comment at the top
#include "dll_boot_logging.hpp"

#include "globals.hpp"
#include "utils/display_commander_logger.hpp"
#include "utils/general_utils.hpp"

// Libraries <standard C++>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>

// Libraries <Windows.h>
#include <Windows.h>

namespace {
bool IsLoaderModule(const wchar_t* path) {
    if (!path || !path[0]) return false;
    const wchar_t* name = path;
    const wchar_t* last = wcsrchr(path, L'\\');
    if (last) name = last + 1;
    const wchar_t* last_slash = wcsrchr(path, L'/');
    if (last_slash && last_slash > name) name = last_slash + 1;
    return _wcsicmp(name, L"ntdll.dll") == 0 || _wcsicmp(name, L"kernel32.dll") == 0
           || _wcsicmp(name, L"kernelbase.dll") == 0 || _wcsicmp(name, L"wow64.dll") == 0
           || _wcsicmp(name, L"wow64win.dll") == 0 || _wcsicmp(name, L"wow64cpu.dll") == 0;
}

void LogBoot(const std::string& text) { AppendDisplayCommanderBootLog(text); }

std::string WideToNarrowCpAcp(std::wstring_view w) {
    if (w.empty()) return {};
    const int len =
        WideCharToMultiByte(CP_ACP, 0, w.data(), static_cast<int>(w.size()), nullptr, 0, nullptr, nullptr);
    if (len <= 0) return "(wide path conversion failed)";
    std::string out(static_cast<size_t>(len), '\0');
    if (WideCharToMultiByte(CP_ACP, 0, w.data(), static_cast<int>(w.size()), out.data(), len, nullptr, nullptr) == 0) {
        return "(wide path conversion failed)";
    }
    return out;
}

std::string WidePathToUtf8(std::wstring_view w) {
    if (w.empty()) return {};
    const int len =
        WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()), nullptr, 0, nullptr, nullptr);
    if (len <= 0) return {};
    std::string out(static_cast<size_t>(len), '\0');
    if (WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()), out.data(), len, nullptr, nullptr) == 0) {
        return {};
    }
    return out;
}

std::string EscapeJsonUtf8(std::string_view s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (unsigned char c : s) {
        switch (c) {
            case '\\':
                out += "\\\\";
                break;
            case '"':
                out += "\\\"";
                break;
            case '\b':
                out += "\\b";
                break;
            case '\f':
                out += "\\f";
                break;
            case '\n':
                out += "\\n";
                break;
            case '\r':
                out += "\\r";
                break;
            case '\t':
                out += "\\t";
                break;
            default:
                if (c < 0x20U) {
                    char buf[8];
                    snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned>(c));
                    out += buf;
                } else {
                    out += static_cast<char>(c);
                }
                break;
        }
    }
    return out;
}

bool Utf8PathToWide(std::string_view utf8, std::wstring& out_w) {
    if (utf8.empty()) {
        out_w.clear();
        return false;
    }
    const int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
    if (n <= 0) return false;
    out_w.assign(static_cast<size_t>(n), L'\0');
    return MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.data(), static_cast<int>(utf8.size()), out_w.data(), n) > 0;
}

// Strip UTF-8 BOM if present.
void StripUtf8Bom(std::string& s) {
    if (s.size() >= 3 && static_cast<unsigned char>(s[0]) == 0xEF && static_cast<unsigned char>(s[1]) == 0xBB
        && static_cast<unsigned char>(s[2]) == 0xBF) {
        s.erase(0, 3);
    }
}

// Parse "display_commander_config_directory" string value (minimal JSON subset).
std::optional<std::wstring> TryReadInstallerMarkerConfigDirectory(const std::filesystem::path& marker_path) {
    std::error_code ec;
    if (!std::filesystem::is_regular_file(marker_path, ec) || ec) {
        return std::nullopt;
    }
    std::ifstream in(marker_path, std::ios::binary);
    if (!in) {
        return std::nullopt;
    }
    std::string json((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    StripUtf8Bom(json);
    constexpr std::string_view kKey = "\"display_commander_config_directory\"";
    const size_t kpos = json.find(kKey);
    if (kpos == std::string::npos) {
        return std::nullopt;
    }
    size_t i = json.find(':', kpos + kKey.size());
    if (i == std::string::npos) {
        return std::nullopt;
    }
    ++i;
    while (i < json.size() && (json[i] == ' ' || json[i] == '\t' || json[i] == '\n' || json[i] == '\r')) {
        ++i;
    }
    if (i >= json.size() || json[i] != '"') {
        return std::nullopt;
    }
    ++i;
    std::string acc;
    while (i < json.size()) {
        const char c = json[i++];
        if (c == '"') {
            break;
        }
        if (c == '\\' && i < json.size()) {
            const char e = json[i++];
            if (e == '"' || e == '\\' || e == '/') {
                acc += e;
            } else if (e == 'n') {
                acc += '\n';
            } else if (e == 'r') {
                acc += '\r';
            } else if (e == 't') {
                acc += '\t';
            } else if (e == 'b') {
                acc += '\b';
            } else if (e == 'f') {
                acc += '\f';
            } else {
                acc += e;
            }
        } else {
            acc += c;
        }
    }
    if (acc.empty()) {
        return std::nullopt;
    }
    std::wstring w;
    if (!Utf8PathToWide(acc, w)) {
        return std::nullopt;
    }
    std::filesystem::path p(w);
    if (p.empty() || !p.is_absolute()) {
        return std::nullopt;
    }
    p = p.lexically_normal();
    if (p.empty()) {
        return std::nullopt;
    }
    return p.wstring();
}

void WriteInstallerMarkerJson(const std::filesystem::path& game_root, const std::wstring& config_dir_w,
                              const std::string& game_display_name, bool config_path_from_marker) {
    if (game_root.empty() || config_dir_w.empty()) {
        return;
    }
    const std::string path_utf8 = WidePathToUtf8(config_dir_w);
    if (path_utf8.empty()) {
        return;
    }
    const std::string game_root_utf8 = WidePathToUtf8(game_root.wstring());
    const std::string path_source = config_path_from_marker ? "marker" : "default";
    const std::string body = std::string("{\"format_version\":1,\"display_commander_config_directory\":\"")
                             + EscapeJsonUtf8(path_utf8) + std::string("\",\"game_display_name\":\"")
                             + EscapeJsonUtf8(game_display_name)
                             + std::string("\",\"display_commander_game_install_root\":\"")
                             + EscapeJsonUtf8(game_root_utf8)
                             + std::string("\",\"display_commander_games_folder_name\":\"")
                             + EscapeJsonUtf8(game_display_name)
                             + std::string("\",\"display_commander_config_path_source\":\"")
                             + EscapeJsonUtf8(path_source) + std::string("\"}\n");
    const std::filesystem::path marker = game_root / L".display_commander_installer_marker.json";
    const std::filesystem::path tmp = game_root / L".display_commander_installer_marker.json.tmp";
    std::error_code ec;
    {
        std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
        if (!out) {
            return;
        }
        out << body;
        out.flush();
        if (!out) {
            std::filesystem::remove(tmp, ec);
            return;
        }
    }
    ec.clear();
    std::filesystem::rename(tmp, marker, ec);
    if (!ec) {
        return;
    }
    std::filesystem::remove(marker, ec);
    ec.clear();
    std::filesystem::rename(tmp, marker, ec);
    if (ec) {
        std::error_code ec_rm;
        std::filesystem::remove(tmp, ec_rm);
        LogBoot("[DC] installer marker: could not write .display_commander_installer_marker.json (permissions?)");
    }
}
}  // namespace

void ChooseAndSetDcConfigPath(HMODULE h_module) {
    std::wstring config_path_w;
    WCHAR module_path[MAX_PATH] = {};
    if (GetModuleFileNameW(h_module, module_path, MAX_PATH) > 0) {
        std::filesystem::path dll_dir = std::filesystem::path(module_path).parent_path();
        std::error_code ec;

        bool use_global_config = false;
        if (std::filesystem::is_regular_file(dll_dir / L".DC_CONFIG_GLOBAL", ec) && !ec) {
            use_global_config = true;
        } else {
            ec.clear();
            std::filesystem::path dc_root = GetDisplayCommanderAppDataRootPathNoCreate();
            if (!dc_root.empty() && std::filesystem::is_regular_file(dc_root / L".DC_CONFIG_GLOBAL", ec) && !ec) {
                use_global_config = true;
            }
        }
        if (use_global_config) {
            std::filesystem::path base = GetDisplayCommanderAppDataFolder();
            if (!base.empty()) {
                std::string game_name = GetGameNameFromProcess();
                if (game_name.empty()) game_name = "Game";
                const std::filesystem::path default_global = base / L"Games" / std::filesystem::path(game_name);
                config_path_w = default_global.wstring();
                bool config_path_from_marker = false;

                const std::filesystem::path game_root = GetGameInstallRootPathFromProcess();
                if (!game_root.empty()) {
                    const auto from_marker =
                        TryReadInstallerMarkerConfigDirectory(game_root / L".display_commander_installer_marker.json");
                    if (from_marker && !from_marker->empty()) {
                        config_path_w = *from_marker;
                        config_path_from_marker = true;
                        LogBoot("[DC] config path from .display_commander_installer_marker.json under game root");
                    }
                }

                ec.clear();
                std::filesystem::create_directories(std::filesystem::path(config_path_w), ec);

                if (!game_root.empty()) {
                    WriteInstallerMarkerJson(game_root, config_path_w, game_name, config_path_from_marker);
                }
            }
        }
        if (config_path_w.empty() &&
            std::filesystem::is_regular_file(dll_dir / L".DC_CONFIG_IN_DLL", ec) && !ec) {
            config_path_w = dll_dir.wstring();
        }
    }
    if (config_path_w.empty()) {
        WCHAR exe_path[MAX_PATH] = {};
        if (GetModuleFileNameW(nullptr, exe_path, MAX_PATH) == 0) return;
        WCHAR* last_slash = wcsrchr(exe_path, L'\\');
        if (last_slash == nullptr || last_slash <= exe_path) return;
        *last_slash = L'\0';
        config_path_w = exe_path;
    }
    if (config_path_w.empty()) return;
    SetEnvironmentVariableW(L"RESHADE_BASE_PATH_OVERRIDE", config_path_w.c_str());
    g_dc_config_directory.store(std::make_shared<std::wstring>(config_path_w));
}

void CaptureDllLoadCallerPath(HMODULE h_our_module) {
    try {
        void* backtrace[256] = {};
        const USHORT n =
            CaptureStackBackTrace(0, static_cast<ULONG>(sizeof(backtrace) / sizeof(backtrace[0])), backtrace, nullptr);
        wchar_t path_buf[MAX_PATH] = {};
        std::string fallback;
        bool fallback_is_loader = false;
        std::string list_buf;
        std::string last_path;
        for (USHORT i = 0; i < n; ++i) {
            HMODULE hmod = nullptr;
            if (!GetModuleHandleExW(
                    GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                    static_cast<LPCWSTR>(backtrace[i]), &hmod)
                || hmod == nullptr) {
                continue;
            }
            if (hmod == h_our_module) continue;
            if (GetModuleFileNameW(hmod, path_buf, MAX_PATH) == 0) continue;
            const std::string path_str = std::filesystem::path(path_buf).string();
            if (path_str != last_path) {
                last_path = path_str;
                if (!list_buf.empty()) list_buf += '\n';
                list_buf += path_str;
            }
            if (fallback.empty()) {
                fallback = path_str;
                fallback_is_loader = IsLoaderModule(path_buf);
            }
            if (IsLoaderModule(path_buf)) continue;
            g_dll_load_caller_path = path_str;
            g_dll_load_call_stack_list = std::move(list_buf);
            return;
        }
        g_dll_load_call_stack_list = std::move(list_buf);
        if (!fallback_is_loader && !fallback.empty()) g_dll_load_caller_path = std::move(fallback);
    } catch (...) {
        // avoid crashing DllMain
    }
}

void LogBootDllMainStage(const char* stage_message) { LogBoot(std::string("[DllMain] ") + stage_message); }

void LogBootRegisterAndPostInitStage(const char* stage_message) {
    LogBoot(std::string("[RegisterAndPostInit] ") + stage_message);
}

void LogBootInitWithoutHwndStage(const char* stage_message) {
    LogBoot(std::string("[InitWithoutHwnd] ") + stage_message);
}

void LogBootDcConfigPath() {
    const auto dc_dir = g_dc_config_directory.load(std::memory_order_acquire);
    if (!dc_dir || dc_dir->empty()) {
        LogBoot("[DC] config path: (not set)");
        return;
    }
    LogBoot("[DC] config path: " + WideToNarrowCpAcp(*dc_dir));
}

void EnsureDisplayCommanderLogWithModulePath(HMODULE h_module) {
    wchar_t module_path_buf[MAX_PATH] = {};
    if (GetModuleFileNameW(h_module, module_path_buf, MAX_PATH) == 0) return;
    char module_path_narrow[MAX_PATH] = {};
    if (WideCharToMultiByte(CP_ACP, 0, module_path_buf, -1, module_path_narrow,
                            static_cast<int>(sizeof(module_path_narrow)), nullptr, nullptr)
        == 0) {
        return;
    }
    char dbg_buf[MAX_PATH + 128];
    int dbg_len = snprintf(dbg_buf, sizeof(dbg_buf), "[DisplayCommander] [Boot] module path: %s", module_path_narrow);
    if (!g_dll_load_caller_path.empty() && dbg_len >= 0 && static_cast<size_t>(dbg_len) < sizeof(dbg_buf) - 32) {
        dbg_len += snprintf(dbg_buf + dbg_len, sizeof(dbg_buf) - static_cast<size_t>(dbg_len), " [Caller] %s",
                            g_dll_load_caller_path.c_str());
    }
    if (dbg_len >= 0 && static_cast<size_t>(dbg_len) < sizeof(dbg_buf)) {
        snprintf(dbg_buf + dbg_len, sizeof(dbg_buf) - static_cast<size_t>(dbg_len), "\n");
        OutputDebugStringA(dbg_buf);
    }
    std::filesystem::path log_path = std::filesystem::path(module_path_buf).parent_path() / "DisplayCommander.log";
    g_dll_main_log_path = log_path.string();

    std::string module_path_line = std::string("[Boot] module path: ") + module_path_narrow;
    if (!g_dll_load_caller_path.empty()) {
        module_path_line += " [Caller] " + g_dll_load_caller_path;
    }

    LogBoot(module_path_line);
}
