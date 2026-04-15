# `Reflex / PCLStats` debug tab: `DC_` ETW session list

## Purpose

Show currently active ETW session names that contain `DC_` in the `Debug -> Reflex / PCLStats` tab, so developers can quickly confirm whether Display Commander-related trace sessions are alive in the current process environment.

This includes both read-only introspection and startup cleanup of stale `DC_` sessions from older runs.

## Scope

- **UI location**: `Debug -> Reflex / PCLStats`
- **Implementation file**: `src/addons/display_commander/ui/new_ui/debug/reflex_pclstats_tab.cpp`
- **Startup cleanup implementation**: `src/addons/display_commander/hooks/nvidia/pclstats_etw_hooks.cpp`
- **Build scope**: Available with the existing debug tab build path (`DEBUG_TABS`), alongside other debug-only views.

## Behavior

- Adds a section labeled **`ETW sessions containing DC_`**.
- Uses `QueryAllTracesW` to enumerate active ETW logger sessions.
- Filters session names by substring match (`contains "DC_"`).
- Renders matching session names in a table.
- Shows total active ETW session count returned by `QueryAllTracesW`.
- Shows the Display Commander owned session name for this process: `DC_PCLStats_<pid>`.

## Startup cleanup

- During PCLStats ETW hook installation, Display Commander enumerates active ETW sessions via `QueryAllTracesW`.
- Any session whose name contains `DC_` and is not the current process owned name (`DC_PCLStats_<pid>`) is stopped via `ControlTraceW(..., EVENT_TRACE_CONTROL_STOP)`.
- The debug UI reports how many stale sessions were removed and the startup query status code.

## Refresh model

- The section keeps a cached snapshot in UI state.
- Snapshot updates when:
  - the section is first shown (first query), or
  - user clicks **`Refresh DC_ ETW sessions`**.

This avoids ETW enumeration on every frame while still allowing explicit re-check.

## Error and empty states

- If ETW query fails (`QueryAllTracesW` returns non-success), the UI shows a dimmed failure line with the Win32 status code.
- If query succeeds but no names match `DC_`, the UI shows a dimmed **none found** message.

## Data formatting

- ETW logger names are received as wide strings and converted to UTF-8 for ImGui rendering using existing utility helpers.
- Matching is done on wide strings before conversion.
