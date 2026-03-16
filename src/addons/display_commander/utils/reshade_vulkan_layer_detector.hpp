#pragma once

// Returns true if the current process executable is listed in ReShade's Vulkan layer
// apps list (C:\ProgramData\ReShade\ReShadeApps.ini [Apps=...]). When true, ReShade
// is registered as a Vulkan layer for this exe.
bool IsReShadeRegisteredAsVulkanLayerForCurrentExe();
