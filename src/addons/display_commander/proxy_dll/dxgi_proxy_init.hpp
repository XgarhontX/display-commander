#pragma once

// Install MinHook on real dxgi.dll CreateDXGIFactory2. Call from addon init (not DllMain).
// No-op if real DXGI not loaded, hook suppression enabled, or already installed.
#if defined(DC_NO_MODULES)
// proxy_dll/*.cpp are not compiled when DC_NO_MODULES (addon-only build); dxgi_proxy.cpp would define this.
inline void InstallRealDXGIMinHookHooks() {}
#else
void InstallRealDXGIMinHookHooks();
#endif
