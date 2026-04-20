#pragma once
#include <entt/entity/fwd.hpp>

/*
 * Non-interactive HUD overlay (ImGui + Vulkan 1.4 + SDL3 + Volk)
 *
 * DESCRIPTOR POOL REQUIREMENTS (your existing pool must include):
 *   - type:            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
 *   - descriptorCount: += 1  (for ImGui)
 *   - maxSets:         += 1  (for ImGui)
 *   - flags:           VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT
 */
namespace System {

// Call after swapchain creation
void hudInit(entt::registry& reg);

// Call after draw calls, before vkCmdEndRendering
// builds and draws the UI
void hudDraw(entt::registry& reg);

// Call before vkDestroyDescriptorPool
void hudShutdown(entt::registry& reg);

}  // namespace System
