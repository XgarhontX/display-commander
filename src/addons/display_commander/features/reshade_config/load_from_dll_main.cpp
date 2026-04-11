// Source Code <Display Commander> // follow this order for includes in all files + add this comment at the top
// Spec: docs/spec/features/reshade_load_from_dll_main.md
#include "load_from_dll_main.hpp"

#include "../../globals.hpp"
#include "../../settings/main_tab_settings.hpp"
#include "../../utils/logging.hpp"

// Libraries <ReShade> / <imgui>
#include <reshade.hpp>

// Libraries <standard C++>
#include <cctype>
#include <string>
#include <string_view>
#include <vector>

// Libraries <Windows.h> — before other Windows headers
#include <Windows.h>

namespace display_commander::features::reshade_config {

namespace {

bool Utf16LeafToUtf8(const wchar_t* wide, std::string& out) {
    if (wide == nullptr || wide[0] == L'\0') {
        return false;
    }
    int needed = WideCharToMultiByte(CP_UTF8, 0, wide, -1, nullptr, 0, nullptr, nullptr);
    if (needed <= 1) {
        return false;
    }
    out.resize(static_cast<size_t>(needed - 1));
    if (WideCharToMultiByte(CP_UTF8, 0, wide, -1, out.data(), needed, nullptr, nullptr) <= 0) {
        out.clear();
        return false;
    }
    return true;
}

bool GetDisplayCommanderModuleBasenameUtf8(std::string& out) {
    wchar_t path[MAX_PATH];
    const DWORD n = GetModuleFileNameW(g_hmodule, path, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) {
        return false;
    }
    const wchar_t* leaf = path;
    for (const wchar_t* p = path; *p != L'\0'; ++p) {
        if (*p == L'\\' || *p == L'/') {
            leaf = p + 1;
        }
    }
    return Utf16LeafToUtf8(leaf, out);
}

bool EqualsIgnoreCaseAscii(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) {
        return false;
    }
    for (size_t i = 0; i < a.size(); ++i) {
        const unsigned char ca = static_cast<unsigned char>(a[i]);
        const unsigned char cb = static_cast<unsigned char>(b[i]);
        if (std::tolower(ca) != std::tolower(cb)) {
            return false;
        }
    }
    return true;
}

bool IsLegacyBoolToken(std::string_view s) {
    return s == "0" || s == "1";
}

void SplitCommaSeparatedTrim(std::string_view single, std::vector<std::string>& out) {
    size_t start = 0;
    while (start < single.size()) {
        size_t comma = single.find(',', start);
        const std::string_view part =
            (comma == std::string::npos) ? single.substr(start) : single.substr(start, comma - start);
        size_t lo = 0;
        size_t hi = part.size();
        while (lo < hi && (part[lo] == ' ' || part[lo] == '\t')) {
            ++lo;
        }
        while (hi > lo && (part[hi - 1] == ' ' || part[hi - 1] == '\t')) {
            --hi;
        }
        if (lo < hi) {
            out.emplace_back(part.substr(lo, hi - lo));
        }
        if (comma == std::string::npos) {
            break;
        }
        start = comma + 1;
    }
}

void ParseLoadFromDllMainBuffer(const char* buffer, size_t buffer_size, std::vector<std::string>& out) {
    out.clear();
    if (buffer == nullptr || buffer_size == 0) {
        return;
    }
    const char* ptr = buffer;
    while (*ptr != '\0' && ptr < buffer + buffer_size) {
        std::string entry(ptr);
        const size_t len = entry.length();
        if (!entry.empty()) {
            out.push_back(std::move(entry));
        }
        ptr += len + 1;
    }

    if (out.size() == 1 && out[0].find(',') != std::string::npos) {
        std::string combined = std::move(out[0]);
        out.clear();
        SplitCommaSeparatedTrim(combined, out);
    }

    std::vector<std::string> filtered;
    filtered.reserve(out.size());
    for (auto& e : out) {
        if (!IsLegacyBoolToken(e)) {
            filtered.push_back(std::move(e));
        }
    }
    out = std::move(filtered);
}

}  // namespace

void MaybeMergeDisplayCommanderIntoReShadeLoadFromDllMainList(reshade::api::effect_runtime* runtime) {
    if (g_reshade_module == nullptr) {
        return;
    }
    if (!settings::g_mainTabSettings.auto_enable_windows_hdr.GetValue()) {
        return;
    }

    std::string basename;
    if (!GetDisplayCommanderModuleBasenameUtf8(basename) || basename.empty()) {
        LogWarn("[ReShadeConfig] Could not resolve Display Commander module basename for LoadFromDllMain");
        return;
    }

    char buffer[4096] = {0};
    size_t buffer_size = sizeof(buffer);
    std::vector<std::string> entries;
    const bool have_key =
        (g_reshade_module != nullptr)
        && reshade::get_config_value(runtime, "ADDON", "LoadFromDllMain", buffer, &buffer_size);

    if (have_key) {
        ParseLoadFromDllMainBuffer(buffer, buffer_size, entries);
    }

    for (const auto& e : entries) {
        if (EqualsIgnoreCaseAscii(e, basename)) {
            return;
        }
    }

    entries.push_back(basename);

    std::string combined;
    for (const auto& path : entries) {
        combined += path;
        combined += '\0';
    }
    reshade::set_config_value(runtime, "ADDON", "LoadFromDllMain", combined.c_str(), combined.size());
    LogInfo("[ReShadeConfig] Added \"%s\" to ADDON LoadFromDllMain for next launch (Auto enable Windows HDR)",
            basename.c_str());
}

}  // namespace display_commander::features::reshade_config
