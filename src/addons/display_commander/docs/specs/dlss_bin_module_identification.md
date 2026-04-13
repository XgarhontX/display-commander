# DLSS model `.bin` module identification

## Purpose

When the NVIDIA App (or runtime) loads DLSS assets, model data can appear as memory-mapped **`.bin`** modules rather than as the usual `nvngx_dlss.dll`, `nvngx_dlssg.dll`, or `nvngx_dlssd.dll` files. Display Commander tracks which NGX-related modules are loaded so the UI and hooks can show paths and behavior consistent with DLSS / DLSS-G / DLSS-D.

This document specifies how we decide that a loaded **`.bin`** image is one of those model blobs and which logical kind it maps to.

## Requirements

1. **Path gate**  
   The module’s file path (from `GetModuleFileNameW`) must contain the substring `NVIDIA\NGX` (case-insensitive), allowing either `\` or `/` between segments.  
   Typical layout: `...\ProgramData\NVIDIA\NGX\models\dlss|dlssg|dlssd\versions\...\files\*.bin`.

   Rationale: avoid scanning arbitrary binaries that happen to embed the same ASCII markers.

2. **Image scan**  
   If the path gate passes, scan the mapped image (up to the first **64 MiB** of `SizeOfImage`) for embedded NUL-terminated style substrings (byte search, Special K–style):

   - `nvngx_dlssg` → **DLSS-G**
   - `nvngx_dlssd` → **DLSS-D** (denoiser / RR)
   - `nvngx_dlss` → **DLSS** (base; checked **after** the more specific names so `nvngx_dlssg` is not mistaken for `nvngx_dlss`)

3. **Failure**  
   If the path gate fails, or `GetModuleInformation` fails, or no needle matches, identification returns “unknown” (no tracking from this path).

## Non-goals

- Parsing PE resources or official NVIDIA container formats.
- Supporting hypothetical future layouts where valid model blobs live **outside** an `NVIDIA\NGX` path (would require a product change / new spec).

## Implementation

- Spec (this file): `src/addons/display_commander/docs/specs/dlss_bin_module_identification.md`
- Code: `src/addons/display_commander/hooks/nvidia/dlss_bin_module_identification.cpp`
- Entry point: `display_commanderhooks::IdentifyDlssBinKind(HMODULE)`
- Call site: `loadlibrary_hooks.cpp` → `OnModuleLoaded` when the file extension is `.bin`

## References

- Tracked module state: `SetDlssTracked` / `GetDlssTrackedPath` in `globals.cpp` (`DlssTrackedKind`).
