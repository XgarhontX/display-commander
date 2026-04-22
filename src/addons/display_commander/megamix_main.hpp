#pragma once
#include <cstdint>
#include <reshade.hpp>

namespace MegaMix {
namespace HighFPS {
    bool OnCreateDevice(reshade::api::device_api api, uint32_t& api_version);
    void OnPresentBefore(reshade::api::command_queue* command_queue, reshade::api::swapchain* swapchain, const reshade::api::rect* source_rect, const reshade::api::rect* dest_rect, uint32_t dirty_rect_count, const reshade::api::rect* dirty_rects);
    void OnPresentAfter(reshade::api::command_queue* queue, reshade::api::swapchain* swapchain);
}}
