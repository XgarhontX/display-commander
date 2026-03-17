# Plan: SafeRemoveAll extension whitelist

**Status: Implemented** (SafeRemoveAll now takes `std::span<const std::wstring> extensions_to_remove`; all call sites updated.)

## Goal

Change `SafeRemoveAll` so it does **not** remove all files in a directory. Instead it will:

1. Accept a **whitelist of file extensions** to remove (e.g. `{ L".dll", L".exe" }`).
2. **Recursively** traverse the directory and remove only **files** whose extension is in the whitelist (comparison case-insensitive).
3. Optionally remove **empty directories** after files are removed (bottom-up so parent dirs become empty and can be removed).

Existing path whitelist (`IsAllowedForRemoveAll`) stays: only allowed roots may be passed to `SafeRemoveAll`. Within those roots, only files matching the extension whitelist are deleted.

---

## Caller-by-caller: path, purpose, and expected extensions

### 1. `main_entry.cpp` — post-ReShade addon temp dirs (4 call sites)

| Call site | Path | Purpose | Expected extensions |
|-----------|------|---------|---------------------|
| ~2192 | `base_dc / L"tmp" / pid_buf` (global_temp_dir) | Temp dir for hardlinked/copied post-ReShade addon DLLs | **`.dc64r`, `.dc32r`, `.dcr`** (64-bit: `.dc64r`+`.dcr`; 32-bit: `.dc32r`+`.dcr`) |
| ~2227 | `addon_dir / L"tmp" / pid_buf` (local_temp_dir) | Same, when cross-volume forces a copy | **`.dc64r`, `.dc32r`, `.dcr`** |
| ~2270 | `base_dc / L"tmp" / pid_str` (CleanupPostReShadeAddonTempDir) | Cleanup of global tmp at exit | **`.dc64r`, `.dc32r`, `.dcr`** |
| ~2276 | `module_path.parent_path() / L"tmp" / pid_str` (module_tmp) | Cleanup of per-module tmp at exit | **`.dc64r`, `.dc32r`, `.dcr`** |

**Single whitelist for main_entry:** `{ L".dc64r", L".dc32r", L".dcr" }`.

---

### 2. `cli_standalone_ui.cpp` — ReShade update temp dir (1 call site)

| Call site | Path | Purpose | Expected extensions |
|-----------|------|---------|---------------------|
| ~303 | `GetTempPath() / L"dc_reshade_update"` | ReShade install/update: download exe, tar extracts DLLs | **`.exe`, `.dll`** (ReShade_Setup_*_Addon.exe, ReShade64.dll, ReShade32.dll) |

**Whitelist:** `{ L".exe", L".dll" }`.

---

### 3. `reshade_version_download.cpp` — ReShade download temp dir (1 call site)

| Call site | Path | Purpose | Expected extensions |
|-----------|------|---------|---------------------|
| ~131 | `GetTempPath() / L"dc_reshade_download"` | ReShade version download: exe, tar extracts DLLs | **`.exe`, `.dll`** (ReShade_Setup_Addon.exe, ReShade64.dll, ReShade32.dll) |

**Whitelist:** `{ L".exe", L".dll" }`.

---

### 4. `version_check.cpp` — Debug staging dir (8 call sites, same dir)

| Call site | Path | Purpose | Expected extensions |
|-----------|------|---------|---------------------|
| 825, 830, 836, 842, 849, 859, 870, 876 | `GetDownloadDirectory() / L"Debug" / L"_staging_latest_debug"` | Staging for downloaded Debug addon (name64, name32 from URL) | **`.dll`** (e.g. Display_Commander_64.dll, Display_Commander_32.dll) |

**Whitelist:** `{ L".dll" }`.

---

## Summary: extensions per caller

| File | Extensions to pass |
|------|---------------------|
| main_entry.cpp | `.dc64r`, `.dc32r`, `.dcr` |
| cli_standalone_ui.cpp | `.exe`, `.dll` |
| reshade_version_download.cpp | `.exe`, `.dll` |
| version_check.cpp | `.dll` |

---

## API design (to implement)

- **Option A — explicit span/vector at each call:**  
  `SafeRemoveAll(path, extensions, ec)`  
  - `extensions`: e.g. `std::span<const std::wstring>` or `const std::vector<std::wstring>&`  
  - Each caller passes the list above.  
  - Pro: explicit at call site. Con: call sites must construct the list (or use a named constant).

- **Option B — overload with “remove all” for backward compatibility:**  
  - `SafeRemoveAll(path, ec)` → treat as “all extensions” (current remove_all behavior) for a transition period, then deprecate or remove.  
  - `SafeRemoveAll(path, extensions, ec)` → new behavior.  
  - Not recommended if we want to eliminate “remove all” entirely.

- **Recommended:** Single signature:  
  `bool SafeRemoveAll(const std::filesystem::path& path, std::span<const std::wstring> extensions_to_remove, std::error_code& ec);`  
  - If `extensions_to_remove` is empty: remove no files (or define: empty = allow no extensions = only remove empty dirs).  
  - Comparison: normalize extension to lowercase before comparing.  
  - Algorithm: recursive directory iterator; for each file, if `extension()` (lowercased) is in the whitelist, `remove(file, ec)`; then (optional) bottom-up remove empty dirs.

---

## Implementation steps

1. **safe_remove.hpp**  
   - Add overload or replace with:  
     `bool SafeRemoveAll(const std::filesystem::path& path, std::span<const std::wstring> extensions_to_remove, std::error_code& ec);`  
   - Document: path must be allowed (`IsAllowedForRemoveAll`), only files with listed extensions are removed, then empty dirs optionally.

2. **safe_remove.cpp**  
   - Implement:  
     - Check path allowed and absolute (unchanged).  
     - Build a set of lowercased extension strings from `extensions_to_remove`.  
     - Recursive traversal: for each regular file, if extension (lowercased) is in the set, `std::filesystem::remove(entry, ec)` and clear ec per file or aggregate.  
     - Optional: after files, walk dirs bottom-up and `remove(dir, ec)` if empty.  
   - Do **not** call `std::filesystem::remove_all`.

3. **Call sites**  
   - **main_entry.cpp:** Define a static const list `{ L".dc64r", L".dc32r", L".dcr" }` and pass it at all 4 call sites.  
   - **cli_standalone_ui.cpp:** Pass `{ L".exe", L".dll" }`.  
   - **reshade_version_download.cpp:** Pass `{ L".exe", L".dll" }`.  
   - **version_check.cpp:** Pass `{ L".dll" }` at all 8 call sites.

4. **Include order**  
   - In any file that gets `<span>`, add it in the standard C++ group; ensure `safe_remove.hpp` does not pull unnecessary includes. If we use `std::vector` instead of `std::span`, use the standard container include.

5. **Tests / CHANGELOG**  
   - CHANGELOG: SafeRemoveAll now takes an extension whitelist and only removes matching files (and optionally empty dirs); list callers and their extensions.  
   - Optional: add a note in `docs/project_structure.md` for `utils/safe_remove.*`.

---

## Edge cases

- **Extension format:** Store and compare with leading dot (e.g. `.dll`). Compare case-insensitive (e.g. `.DLL` matches).
- **Files with no extension:** Only removed if the whitelist explicitly contains empty string `L""` (if we support that). Otherwise leave as-is; none of the current callers need to remove extensionless files.
- **Symlinks/junctions:** Define whether to remove only the link or follow. Recommendation: remove only the link (same as `remove()`), do not follow.
- **Empty `extensions_to_remove`:** Define: remove no files; optionally still remove empty directories. Callers should never pass empty unless that behavior is desired.
