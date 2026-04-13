# Specification: `settings_wrapper` (`settings_wrapper.hpp` / `settings_wrapper.cpp`)

## 1. Purpose

`ui::new_ui::settings_wrapper` defines **typed configuration values** bound to Display Commander’s config system and **ImGui control helpers** that draw sliders, checkboxes, combos, and radio groups with a consistent **“reset to default”** affordance.

**Sources:** `src/addons/display_commander/ui/new_ui/settings_wrapper.hpp`, `settings_wrapper.cpp`.

## 2. Dependencies

| Dependency | Role |
|------------|------|
| `display_commander::config::*` | Read/write INI (and overrides where noted). |
| `display_commander::config::get_config_value_or_default` | Effective defaults for `ComboSettingEnum::GetDefaultValue()` and some loads. |
| `display_commander::config::SetGlobalOverrideValue` | `OverrideBoolSetting::Save()` target. |
| `display_commander::config::save_config` | Persist after user-driven `SetValue` on most types. |
| `display_commander::ui::IImGuiWrapper` | All UI helpers; no direct `ImGui::` in `.cpp` except via wrapper (implementation uses ReShade wrapper). |
| `../forkawesome.h` | `ICON_FK_UNDO` on reset controls. |

## 3. Constants

- **`DEFAULT_SECTION`**: `"DisplayCommander"` — default INI section for keys unless a setting is constructed with another section.

## 4. `SettingBase`

- **Key / section:** `GetKey()`, `GetSection()` identify the config entry (or entries for composite types).
- **Dirty flags:** `is_dirty_` exists; not all subclasses drive it the same way (see `StringSetting`, `FixedIntArraySetting`).
- **Virtuals:** `Load()`, `Save()`, `GetValueAsString()` — each concrete type implements persistence and string form for logging/comparison.

## 5. Scalar settings (atomic-backed)

### 5.1 `FloatSetting`

- **Storage:** `std::atomic<float>`.
- **Load:** If key present: reject non-finite or `< min_`; if `> max_`, either keep value (`preserve_over_max_on_load_`) or clamp to safe default; else apply `get_config_value_or_default` and clamp to `[min_, max_]`.
- **Save:** Writes float to `section.key`.
- **`SetValue`:** Clamps to `[min_, max_]`, stores, then **`save_config`** with a reason string including section/key when not default section.
- **`SetMax`:** Runtime cap adjustment (e.g. overlay spacing max from display size).

### 5.2 `IntSetting`

- **Load:** Integer from config; if out of `[min_, max_]`, reset to clamped default and save.
- **Else:** `get_config_value_or_default` path for effective default.
- **`SetValue`:** Clamp, save, `save_config` with reason.

### 5.3 `BoolSetting`

- **Load:** Only `0` or `1` accepted; otherwise revert to default and save.
- **Else:** bool from `get_config_value_or_default`.
- **Save:** `1` / `0`.

### 5.4 `OverrideBoolSetting`

- **Load:** Same strict `0`/`1` rule as `BoolSetting` when key exists; else `get_config_value_or_default`.
- **Save:** **`SetGlobalOverrideValue`** (global overrides file), not the main INI path used by other bools.

## 6. Reference-backed settings (`*Ref`)

External `std::atomic<>` owned elsewhere; this wrapper syncs config ↔ atomic.

- **`BoolSettingRef`**, **`FloatSettingRef`**, **`IntSettingRef`:** Load/Save mirror scalar types; `SetValue` updates ref and calls `save_config`.
- **`FloatSettingRef` / `IntSettingRef`:** Optional **dirty** fields (`dirty_value_`, `has_dirty_value_`) for slider live preview — **not** written by `settings_wrapper.cpp` load/save; consumers use the API on the class.

## 7. Combo settings

### 7.1 `ComboSetting`

- **Storage:** Plain `int` index (not atomic).
- **Load:** If index out of `[0, labels_.size()-1]`, clamp default (with bounds fix for bad `default_value_`).
- **`SetValue`:** Clamps to valid index range, save, `save_config`.

### 7.2 `ComboSettingRef`

- Same index rules; value lives in external `std::atomic<int>`.

### 7.3 `ComboSettingEnum<EnumType>`

- Same as combo index storage; `GetEnumValue()` casts for typed use.
- **`GetDefaultValue()`:** Uses **`get_config_value_or_default`** for section/key vs `default_value_`, then clamps to label range — supports **per-game default overrides** for reset UI.

## 8. Composite settings

### 8.1 `ResolutionPairSetting`

- Keys: `{key}_width`, `{key}_height`.
- **`SetCurrentResolution`:** `(0,0)` sentinel for “current” (documented in header).

### 8.2 `RefreshRatePairSetting`

- Keys: `{key}_num`, `{key}_denum`.
- **`GetHz()`:** `numerator/denominator`; denominator `0` means “current” (returns `0.0`).

### 8.3 `FixedIntArraySetting`

- Keys: `{key}_0`, `{key}_1`, …
- **Storage:** Heap-allocated `std::atomic<int>` per element (constructor allocates; destructor deletes).
- **Load:** Per-element clamp to `[min_, max_]`, logs each element.
- **Save:** Only when `is_dirty_`; per-element write.
- **`SetValue` / `SetAllValues`:** Clamp, mark dirty, may save immediately depending on path.

### 8.4 `StringSetting`

- **Load:** Up to 256-byte buffer from config; on miss uses `default_value_`.
- **Save:** Only when `is_dirty_` after `SetValue`.

## 9. ImGui wrapper API (`.cpp`)

All helpers return **`true`** if the backing setting’s value **changed** (including reset-to-default).

### 9.1 Reset control (`ResetToDefaultButtonExtent`)

- **Location:** Anonymous namespace in `settings_wrapper.cpp`.
- **Formula:** `GetTextLineHeight() + (FramePadding.y * 2)` — matches ImGui **frame height** so icon-only reset buttons align with sliders and avoid **clipping** ForkAwesome glyphs vs `SmallButton`.
- **Control:** `Button(ICON_FK_UNDO, ImVec2(ext, ext))` after `SameLine()`.

### 9.2 `SliderFloatSetting` / `SliderIntSetting`

- Draw slider; if current value ≠ default (float uses `fabsf` vs `1e-6f`), show reset button in a nested `BeginGroup` / `PushID(&setting)`.

### 9.3 `CheckboxSetting` (both overloads)

- Checkbox; if ≠ default, reset button on same line.

### 9.4 `ComboSettingWrapper`

- Optional `combo_width` via `SetNextItemWidth`.
- Reset uses default label in tooltip when index valid.

### 9.5 `ComboSettingEnumWrapper`

- Optional **`label_text_color`:** If non-null, combo label is `##enumcombo` and the visible title is `TextColored` after the combo; reset `PushID` uses `1` vs `stack_id` to avoid collisions.

### 9.6 `RadioSettingEnumWrapper`

- Optional `group_label` + hover tooltip.
- Radio indices in loop with per-item `PushID(i)`; horizontal layout uses `SameLine` with `GetStyleItemSpacingX()`.
- Reset button `PushID(count)` after radios.

### 9.7 `RadioSettingLayout`

- `Horizontal` | `Vertical` — only horizontal uses `SameLine` between radios.

## 10. Template instantiations (`settings_wrapper.cpp`)

Explicit instantiations are provided for:

`ScreensaverMode`, `OnPresentReflexMode`, `FrameTimeMode`, `WindowMode`, `InputBlockingMode`, `LogLevel`, `FpsLimiterPreset`, `OverlayLabelMode`

— both `ComboSettingEnum` and `ComboSettingEnumWrapper`, and `RadioSettingEnumWrapper` for **`OverlayLabelMode`** and **`ScreensaverMode`** only.

Other enum types get `ComboSettingEnumWrapper` but **not** `RadioSettingEnumWrapper` unless added later.

## 11. `LoadTabSettingsWithSmartLogging`

- For each `SettingBase*`: snapshot `GetValueAsString()`, call `Load()`, compare to new string.
- Logs tab name and list of **`key old->new`** for entries that changed, or “all values at default”.

## 12. Threading and persistence notes

- **Atomics:** Scalar and ref types expose `GetAtomic()` for lock-free reads elsewhere; `SetValue` still performs config IO — callers should not assume lock-free writes across threads without higher-level coordination.
- **`save_config`:** Invoked frequently on `SetValue` for many types; batching is not done here.

## 13. Non-goals (current code)

- No validation of `labels_` lifetime (callers must keep string storage alive for combo labels).
- `FixedIntArraySetting` does not use `std::vector<std::atomic<int>>` (uses pointers by design in current code).
- UI helpers do not support “revert without save” beyond not calling `SetValue` until commit (dirty refs are separate).

## 14. Change checklist

When adding a new `ComboSettingEnum` user:

1. Add template instantiation lines at bottom of `settings_wrapper.cpp` if the type is used from other TUs.
2. If radio reset is needed, add `RadioSettingEnumWrapper` instantiation.
3. Ensure config keys and `get_config_value_or_default` behavior match tab `LoadSettings` expectations.
