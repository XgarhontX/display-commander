#pragma once

// Source Code <Display Commander>

#include <cstdint>

#include <windows.h>

// Streamline hook functions
bool InstallStreamlineHooks(HMODULE streamline_module = nullptr);

// Get last SDK version from slInit calls
uint64_t GetLastStreamlineSDKVersion();

// Session call count for slUpgradeInterface (Streamline loader hook)
uint64_t GetSlUpgradeInterfaceCallCount();

// Per-session counts: COM type detected on successful slUpgradeInterface (QueryInterface order), plus edge cases.
uint64_t GetSlUpgradeInterfaceClassCountFactory();
uint64_t GetSlUpgradeInterfaceClassCountSwapChain();
uint64_t GetSlUpgradeInterfaceClassCountD3D11Device();
uint64_t GetSlUpgradeInterfaceClassCountD3D12Device();
uint64_t GetSlUpgradeInterfaceClassCountUnknown();
uint64_t GetSlUpgradeInterfaceClassifyNonOkCount();
uint64_t GetSlUpgradeInterfaceClassifyNullIfaceCount();
