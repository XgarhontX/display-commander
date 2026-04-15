#pragma once

// Libraries <Windows.h> — before other Windows headers
#include <Windows.h>

/** Install advapi32 EventRegister + EventWriteTransfer detours to detect foreign PCLStats ETW setup. */
bool InstallPCLStatsEtwHooks(HMODULE advapi32_module);

void UninstallPCLStatsEtwHooks();

bool ArePCLStatsEtwHooksInstalled();

/**
 * True if another module registered PCLStatsTraceLoggingProvider or emitted PCLStatsInit via EventWriteTransfer
 * (observed after hooks were installed). Used to skip Display Commander's own PCLSTATS_INIT.
 */
bool PclStatsForeignInitObserved();

/** Reserved DC-owned ETW session name for this process (UTF-8). */
const char* GetPCLStatsOwnedEtwSessionName();

/** Number of stale DC_ ETW sessions stopped during startup cleanup. */
ULONG GetPCLStatsDcEtwCleanupRemovedCount();

/** Last startup cleanup status (Win32). */
ULONG GetPCLStatsDcEtwCleanupStatus();
