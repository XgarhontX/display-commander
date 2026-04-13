// Source Code <Display Commander> // follow this order for includes in all files + add this comment at the top
#include "dlss_bin_module_identification.hpp"

// Libraries <standard C++>
#include <algorithm>
#include <cstring>
#include <string>

// Libraries <Windows.h>
#include <Windows.h>

// Libraries <Windows>
#include <psapi.h>

namespace display_commanderhooks {

namespace {

// True if the module was loaded from under an NVIDIA NGX directory (e.g. ProgramData\NVIDIA\NGX\models\...).
// See display_commander/docs/specs/dlss_bin_module_identification.md (path gate).
bool DlssBinModulePathUnderNgx(HMODULE hMod) {
    wchar_t path_buf[MAX_PATH];
    if (GetModuleFileNameW(hMod, path_buf, MAX_PATH) == 0) {
        return false;
    }
    std::wstring path(path_buf);
    std::transform(path.begin(), path.end(), path.begin(), ::towlower);
    return path.find(L"nvidia\\ngx") != std::wstring::npos || path.find(L"nvidia/ngx") != std::wstring::npos;
}

}  // namespace

std::optional<DlssTrackedKind> IdentifyDlssBinKind(HMODULE hMod) {
    // Spec: display_commander/docs/specs/dlss_bin_module_identification.md
    if (!DlssBinModulePathUnderNgx(hMod)) {
        return std::nullopt;
    }
    MODULEINFO modInfo = {};
    if (!GetModuleInformation(GetCurrentProcess(), hMod, &modInfo, sizeof(modInfo)) || modInfo.SizeOfImage == 0) {
        return std::nullopt;
    }
    const char* base = static_cast<const char*>(modInfo.lpBaseOfDll);
    const size_t size = modInfo.SizeOfImage;
    // Cap scan at 64 MiB to avoid long scans on huge images
    const size_t scan_size = (size > 64 * 1024 * 1024) ? (64 * 1024 * 1024) : size;

    auto find_str = [base, scan_size](const char* needle) -> bool {
        const size_t nlen = std::strlen(needle);
        if (nlen == 0 || nlen > scan_size) return false;
        for (size_t i = 0; i + nlen <= scan_size; ++i) {
            if (std::memcmp(base + i, needle, nlen) == 0) return true;
        }
        return false;
    };

    if (find_str("nvngx_dlssg")) return DlssTrackedKind::DLSSG;
    if (find_str("nvngx_dlssd")) return DlssTrackedKind::DLSSD;
    if (find_str("nvngx_dlss")) return DlssTrackedKind::DLSS;
    return std::nullopt;
}

}  // namespace display_commanderhooks
