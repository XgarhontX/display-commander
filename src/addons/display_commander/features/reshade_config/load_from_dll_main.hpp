// Source Code <Display Commander> // follow this order for includes in all files + add this comment at the top
// Spec: docs/spec/features/reshade_load_from_dll_main.md
#pragma once

namespace reshade::api {
struct effect_runtime;
}

namespace display_commander::features::reshade_config {

/// If Auto enable Windows HDR is on, ensures ReShade [ADDON] LoadFromDllMain lists this module's filename
/// (basename). Persists to ReShade.ini for the next process start; see spec.
void MaybeMergeDisplayCommanderIntoReShadeLoadFromDllMainList(reshade::api::effect_runtime* runtime);

}  // namespace display_commander::features::reshade_config
