// Source Code <Display Commander> // follow this order for includes in all files + add this comment at the top
#pragma once

// Libraries <standard C++>
#include <cstdint>

namespace display_commander::features::presentmon {

enum class PresentMonMode : uint32_t {
    Unknown = 0,
    ComposedFlip = 1,
};

struct PresentMonStateSnapshot {
    PresentMonMode mode = PresentMonMode::Unknown;
    bool has_data = false;
    bool is_live = false;
    bool session_running = false;
    bool session_failed = false;
    uint64_t age_ms = 0;
};

// Start ETW session lazily (idempotent).
void EnsurePresentMonEtwStarted();

// Best-effort stop for process shutdown.
void ShutdownPresentMonEtw();

// Current-process-only state snapshot for UI.
PresentMonStateSnapshot GetPresentMonStateSnapshot();

const char* PresentMonModeToString(PresentMonMode mode);

}  // namespace display_commander::features::presentmon

