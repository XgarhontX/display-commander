#pragma once

// Source Code <Display Commander> // follow this order for includes in all files + add this comment at the top
#include "../../globals.hpp"

namespace display_commanderhooks {

// Identify a mapped `.bin` as DLSS / DLSS-G / DLSS-D model data (NVIDIA NGX path + image scan).
// Spec: docs/specs/dlss_bin_module_identification.md (root docs/ checkout, sibling to src/)
std::optional<DlssTrackedKind> IdentifyDlssBinKind(HMODULE hMod);

}  // namespace display_commanderhooks
