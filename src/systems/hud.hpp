/*
 * Non-interactive HUD overlay (ImGui + Vulkan 1.4 + SDL3 + Volk)
 *
 * DESCRIPTOR POOL REQUIREMENTS (your existing pool must include):
 *   - type:            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
 *   - descriptorCount: += 1  (for ImGui)
 *   - maxSets:         += 1  (for ImGui)
 *   - flags:           VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT
 */

#pragma once
#include <entt/entity/fwd.hpp>

namespace sys {

// Call after swapchain creation
void hud_init(entt::registry& reg);

// Call top of render loop, before polling
void hud_begin(entt::registry& reg);

// Call after draw calls, before vkCmdEndRendering
// builds and draws the UI
void hud_draw(entt::registry& reg);

// Call before vkDestroyDescriptorPool
void hud_shutdown(entt::registry& reg);

}  // namespace sys
