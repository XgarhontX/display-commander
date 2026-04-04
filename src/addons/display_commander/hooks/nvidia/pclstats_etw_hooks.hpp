#pragma once

#include <windows.h>

/** Install advapi32 EventRegister + EventWriteTransfer detours to detect foreign PCLStats ETW setup. */
bool InstallPCLStatsEtwHooks(HMODULE advapi32_module);

void UninstallPCLStatsEtwHooks();

bool ArePCLStatsEtwHooksInstalled();

/**
 * True if another module registered PCLStatsTraceLoggingProvider or emitted PCLStatsInit via EventWriteTransfer
 * (observed after hooks were installed). Used to skip Display Commander's own PCLSTATS_INIT.
 */
bool PclStatsForeignInitObserved();
