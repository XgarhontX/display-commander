#pragma once

namespace reshade::api {
class effect_runtime;
}

void OverrideReShadeSettings(reshade::api::effect_runtime* runtime = nullptr);
