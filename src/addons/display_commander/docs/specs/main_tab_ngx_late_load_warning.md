# Main tab: NGX “loaded too late” warning spec

## Purpose

Surface a **prominent warning on the Main tab** when Display Commander has installed NGX **export** hooks on `_nvngx.dll` but **NGX Parameter vtable hooks** were never installed. That combination usually means DC loaded after NGX already initialized, so Parameter objects were created without going through our detours—overrides and DLSS preset controls may not apply.

## Scope

- **NGX state**: [`src/addons/display_commander/hooks/nvidia/ngx_hooks.hpp`](../../hooks/nvidia/ngx_hooks.hpp), [`ngx_hooks.cpp`](../../hooks/nvidia/ngx_hooks.cpp) — `InstallNGXHooks`, `AreNGXInitializationHooksInstalled`, `AreNGXParameterVTableHooksInstalled`, `HookNGXParameterVTable`.
- **DLSS section visibility proxy**: [`src/addons/display_commander/globals.hpp`](../../globals.hpp), [`globals.cpp`](../../globals.cpp) — `ShouldShowDlssInformationSection()`.
- **UI**: [`src/addons/display_commander/ui/new_ui/main_new_tab.cpp`](../../ui/new_ui/main_new_tab.cpp) — warning banner near other `ui:tab:main_new:warnings:*` blocks.
- **Related UI** (unchanged behavior): [`src/addons/display_commander/ui/new_ui/controls/panel_dlss_control.cpp`](../../ui/new_ui/controls/panel_dlss_control.cpp) — DXGI-only inline warning inside the DLSS Control collapsible.

## Terminology

- **NGX initialization (export) hooks**: MinHook detours on `_nvngx.dll` exports (e.g. `GetParameters`, `Init`, `CreateFeature`). Installed once when `InstallNGXHooks` succeeds (not suppressed, non-null module).
- **Parameter vtable hooks**: Hooks on the `NVSDK_NGX_Parameter` vtable installed inside detours when a Parameter instance is first seen — `HookNGXParameterVTable` sets `g_ngx_vtable_hooks_installed` after success.

## Functional behavior

### When to show the Main tab warning

Show the banner when **all** of:

1. `AreNGXInitializationHooksInstalled()` is true.
2. `AreNGXParameterVTableHooksInstalled()` is false.
3. `ShouldShowDlssInformationSection()` is true.

**Rationale for (3):** Without DLSS/NGX activity (or related modules) in the session, `!AreNGXParameterVTableHooksInstalled()` alone can be true simply because no Parameter object has been created yet. Gating on `ShouldShowDlssInformationSection()` aligns with the existing DLSS Control warning and reduces false positives.

### UI copy

- **Visible line** (warning color, warning icon): short message that DC was loaded too late for NGX parameter hooks and to consider loading DC as a DLL proxy.
- **Tooltip**: Same guidance as the DLSS Control panel: ReShade loading order, `_nvngx.dll` already loaded, recommendation to use DC as `dxgi.dll` / `d3d12.dll` / `d3d11.dll` / `version.dll` with ReShade as `Reshade64.dll`, and that parameter overrides / preset controls may not apply.

### API / graphics scope

- The Main tab banner is **not** limited to DXGI APIs (unlike the collapsible DLSS Control warning). Vulkan sessions can hit the same late-load pattern.

### Observability

- `g_rendering_ui_section` key: `ui:tab:main_new:warnings:ngx_late_load`.

## Non-goals

- Do **not** treat `GetModuleHandleW(L"_nvngx.dll") != nullptr` alone as “hooks installed”; hooks may be suppressed or install may fail.
- Do **not** remove or replace the DLSS Control subsection warning; the Main tab adds visibility for users who never open that section.
- **`CleanupNGXHooks`**: Called on device destroy for handle tracking; it does **not** uninstall export hooks. The init-hook-installed flag is **not** cleared there (hooks remain active for the process).

## Relation to existing APIs

- `AreNGXInitializationHooksInstalled()` mirrors the former function-local `g_ngx_hooks_installed` in `InstallNGXHooks` (file-scope `g_ngx_init_hooks_installed`).
- `AreNGXParameterVTableHooksInstalled()` reflects whether `HookNGXParameterVTable` completed at least once.

## Acceptance criteria

1. With NGX export hooks installed, DLSS/NGX activity visible per `ShouldShowDlssInformationSection()`, and vtable hooks never installed, the Main tab shows the warning and tooltip.
2. After vtable hooks install successfully, the warning disappears.
3. If NGX hooks are suppressed or never installed, the warning does not appear solely because `_nvngx.dll` is loaded.
4. If there is no DLSS/NGX section activity, the warning does not appear only because vtable hooks are not yet installed.
