#pragma once

#include <windows.h>

typedef BOOL(WINAPI* wglSwapBuffers_pfn)(HDC hdc);

extern wglSwapBuffers_pfn wglSwapBuffers_Original;

bool InstallOpenGLHooks(HMODULE hModule);
void UninstallOpenGLHooks();

BOOL WINAPI wglSwapBuffers_Detour(HDC hdc);
