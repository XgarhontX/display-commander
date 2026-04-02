#pragma once

// Libraries <standard C++>
#include <string>

// Libraries <Windows.h>
#include <Windows.h>

namespace display_commanderhooks {

// Minimal surface for the main addon (timing, settings): QPC originals + QPC module list load.
using QueryPerformanceCounter_pfn = BOOL(WINAPI*)(LARGE_INTEGER* lpPerformanceCount);
using QueryPerformanceFrequency_pfn = BOOL(WINAPI*)(LARGE_INTEGER* lpFrequency);

extern QueryPerformanceCounter_pfn QueryPerformanceCounter_Original;
extern QueryPerformanceFrequency_pfn QueryPerformanceFrequency_Original;

void LoadQPCEnabledModulesFromSettings(const std::string& enabled_modules_str);

}  // namespace display_commanderhooks
