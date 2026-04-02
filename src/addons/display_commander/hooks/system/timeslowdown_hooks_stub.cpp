// Source Code <Display Commander> // follow this order for includes in all files + add this comment at the top
#include "timeslowdown_hooks.hpp"


// Libraries <standard C++>
#include <string>

namespace display_commanderhooks {

// Full timeslowdown TU is omitted when DC_EXTERNAL_MODULES=OFF; provide globals and minimal API so the addon links.
// Hook targets stay nullptr — callers use fallbacks (e.g. QueryPerformanceFrequency, GetTickCount64).

QueryPerformanceCounter_pfn QueryPerformanceCounter_Original = QueryPerformanceCounter;
QueryPerformanceFrequency_pfn QueryPerformanceFrequency_Original = QueryPerformanceFrequency;

void LoadQPCEnabledModulesFromSettings(const std::string& enabled_modules_str) { (void)enabled_modules_str; }


}  // namespace display_commanderhooks
