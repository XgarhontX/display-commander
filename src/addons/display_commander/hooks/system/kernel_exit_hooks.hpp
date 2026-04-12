// Source Code <Display Commander> // MinHook detours on kernel32 ExitProcess / TerminateProcess; see hooks/system/process_exit_hooks.cpp
#pragma once

namespace display_commanderhooks {

bool InstallProcessExitHooks();
void UninstallProcessExitHooks();

}  // namespace display_commanderhooks
