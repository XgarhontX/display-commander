// Source Code <Display Commander> // follow this order for includes in all files + add this comment at the top
#include "modules/example_dummy/example_dummy_module.hpp"

// Source Code <Display Commander>

// Libraries <standard C++>
#include <atomic>
#include <cstdint>

namespace modules::example_dummy {
namespace {

std::atomic<std::uint64_t> g_tick_counter{0};

}  // namespace

void Initialize(modules::ModuleConfigApi* config_api) {
    (void)config_api;
    g_tick_counter.store(0, std::memory_order_relaxed);
}

void Tick() {
    g_tick_counter.fetch_add(1, std::memory_order_relaxed);
}

void DrawTab(display_commander::ui::IImGuiWrapper& imgui, reshade::api::effect_runtime* runtime) {
    (void)runtime;
    const std::uint64_t tick_count = g_tick_counter.load(std::memory_order_relaxed);
    imgui.Text("Example Dummy Module");
    imgui.TextWrapped("This is a minimal in-repo example module showing initialize/tick/tab/overlay callbacks.");
    imgui.Separator();
    imgui.Text("Tick counter: %llu", static_cast<unsigned long long>(tick_count));
}

void DrawOverlay(display_commander::ui::IImGuiWrapper& imgui) {
    const std::uint64_t tick_count = g_tick_counter.load(std::memory_order_relaxed);
    imgui.Text("Example Dummy: ticks=%llu", static_cast<unsigned long long>(tick_count));
}

}  // namespace modules::example_dummy
