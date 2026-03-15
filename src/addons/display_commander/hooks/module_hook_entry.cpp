// Source Code <Display Commander> // follow this order for includes in all files
#include "module_hook_entry.hpp"
#include "../utils/general_utils.hpp"
#include "../utils/logging.hpp"

// Libraries <standard C++>
#include <cstddef>

// Libraries <Windows.h>
#include <Windows.h>

// Libraries <Windows> / SDK
#include <MinHook.h>

namespace display_commanderhooks {

void RemoveHooksFromModule(void* hModule, const HookEntry* entries, std::size_t count) {
    auto* const module = static_cast<HMODULE>(hModule);
    for (std::size_t i = 0; i < count; ++i) {
        const HookEntry& entry = entries[i];
        FARPROC target = GetProcAddress(module, entry.name);
        if (target != nullptr) {
            MH_RemoveHook(reinterpret_cast<LPVOID>(target));
        }
    }
}

void ResetOriginalsFromTable(const HookEntry* entries, std::size_t count) {
    for (std::size_t i = 0; i < count; ++i) {
        if (entries[i].original != nullptr) {
            *entries[i].original = nullptr;
        }
    }
}

bool InstallHooksFromModule(void* hModule, const HookEntry* entries, std::size_t count) {
    auto* const module = static_cast<HMODULE>(hModule);
    for (std::size_t i = 0; i < count; ++i) {
        const HookEntry& entry = entries[i];
        FARPROC target = GetProcAddress(module, entry.name);
        if (target == nullptr) {
            LogError("%s not found in module", entry.name);
            return false;
        }
        if (!CreateAndEnableHook(reinterpret_cast<LPVOID>(target), entry.detour, entry.original,
                                entry.name)) {
            LogError("Failed to create and enable %s hook", entry.name);
            return false;
        }
    }
    return true;
}

void InstallHooksFromResolver(const HookEntry* entries, std::size_t count, HookResolverFn resolve) {
    for (std::size_t i = 0; i < count; ++i) {
        const HookEntry& entry = entries[i];
        void* target = resolve(entry.name);
        if (target == nullptr) {
            LogInfo("%s not available", entry.name);
            continue;
        }
        if (!CreateAndEnableHook(static_cast<LPVOID>(target), entry.detour, entry.original,
                                entry.name)) {
            LogWarn("Failed to create and enable %s hook", entry.name);
        }
    }
}

void RemoveHooksByOriginalTable(const HookEntry* entries, std::size_t count) {
    for (std::size_t i = 0; i < count; ++i) {
        const HookEntry& entry = entries[i];
        if (entry.original != nullptr && *entry.original != nullptr) {
            MH_DisableHook(*entry.original);
            MH_RemoveHook(*entry.original);
        }
    }
}

}  // namespace display_commanderhooks
