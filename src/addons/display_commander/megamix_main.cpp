#include <dxgi.h>
#include <cstdint>
#include <bit>
#include <reshade.hpp>
#include "globals.hpp"
#include "settings/main_tab_settings.hpp"

namespace MegaMix {
namespace HighFPS {
   //https://github.com/SpecialKO/SpecialK/blob/6fe51ee1eca4aee26a59e227ee5402ad3b55fcc0/src/plugins/unclassified.cpp#L1264

   static uint32_t* addr_puiGameLimit;
   static uintptr_t* addr_menuFlagPtr;

   static bool IsReady()
   {
      return addr_puiGameLimit != nullptr && addr_menuFlagPtr != nullptr;
   }

   static void Init()
   {
      auto base = std::bit_cast<uintptr_t>(GetModuleHandleA("DivaMegaMix.exe"));
      if (base == 0) {
         reshade::log::message(reshade::log::level::error, "Megamix High FPS: Failed to get base address of DivaMegaMix.exe");
         return;
      }

      addr_puiGameLimit = std::bit_cast<uint32_t*>(base + 0x14ABBB8);
      addr_menuFlagPtr = std::bit_cast<uintptr_t*>(base + 0x11481E8); //to object

      reshade::log::message(reshade::log::level::info, "Megamix High FPS: Init() success.");
   }

   //from obj @ ptr addr
   static bool IsMenu()
   {
      uintptr_t obj = *addr_menuFlagPtr;
      if (obj == 0) return false;
      return (*std::bit_cast<uint8_t*>(obj + 0x780) & 0x1) != 0;
   }

   static // must be per frame update/patch as the game forces and reset to 60
   void Patch()
   {
      if (!IsReady()) return;
      auto target = 0u;
      if (settings::g_mainTabSettings.megamix__is_menu_fps_clamp.GetValue() && IsMenu()) target = 60u;
      *addr_puiGameLimit = target;
   }

   static void Unpatch()
   {
      if (!IsReady()) return;
      *addr_puiGameLimit = 60u;
   }

   bool OnCreateDevice(reshade::api::device_api /* api */, uint32_t& /* api_version */) {
      Init();
      return true;
   }

   void OnPresentBefore(reshade::api::command_queue* /* command_queue */, reshade::api::swapchain* /* swapchain */, const reshade::api::rect* /*source_rect*/, const reshade::api::rect* /*dest_rect*/, uint32_t /*dirty_rect_count*/, const reshade::api::rect* /*dirty_rects*/)
   {
      Patch();
   }

   void OnPresentAfter(reshade::api::command_queue* /* queue */, reshade::api::swapchain* /* swapchain */) {
      Patch();
   }
}}