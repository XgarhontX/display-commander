// Source Code <Display Commander> // follow this order for includes in all files + add this comment at the top
// DC_LITE: full ETW implementation is excluded; these no-ops satisfy links from swapchain/exit paths.

#include "presentmon_minimal_etw.hpp"

namespace display_commander::features::presentmon {

void EnsurePresentMonEtwStarted() {}

void ShutdownPresentMonEtw() {}

PresentMonStateSnapshot GetPresentMonStateSnapshot() {
    PresentMonStateSnapshot snapshot{};
    snapshot.session_failed = true;
    return snapshot;
}

const char* PresentMonModeToString(PresentMonMode mode) {
    switch (mode) {
        case PresentMonMode::ComposedFlip: return "Composed: Flip";
        case PresentMonMode::Unknown:
        default: return "Unknown";
    }
}

}  // namespace display_commander::features::presentmon
