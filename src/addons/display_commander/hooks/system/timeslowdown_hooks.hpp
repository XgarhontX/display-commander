#pragma once

#include <windows.h>
#include <string>

namespace display_commanderhooks {
// Function pointer types for timeslowdown hooks
using QueryPerformanceCounter_pfn = BOOL(WINAPI *)(LARGE_INTEGER *lpPerformanceCount);
using QueryPerformanceFrequency_pfn = BOOL(WINAPI *)(LARGE_INTEGER *lpFrequency);

// Timeslowdown hook function pointers
extern QueryPerformanceCounter_pfn QueryPerformanceCounter_Original;
extern QueryPerformanceFrequency_pfn QueryPerformanceFrequency_Original;



void LoadQPCEnabledModulesFromSettings(const std::string& enabled_modules_str);

} // namespace display_commanderhooks
