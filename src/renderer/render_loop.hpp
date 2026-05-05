#pragma once
#include <entt/entity/fwd.hpp>

namespace RenderLoop {
void init(entt::registry& registry);  // Cache all the pointers/entities once

void waitForFences(entt::registry& registry);
void acquireNextImage(entt::registry& registry);  // returns true if swapchain needs rebuild
void updateShaderData(entt::registry& registry);
void updateInstanceBuffer(entt::registry& registry);
void recordCommandBuffer(entt::registry& registry);
void submitCommandBuffer(entt::registry& registry);
void advanceFrameIndex(entt::registry& registry);
void presentImage(entt::registry& registry);      // returns true if swapchain needs rebuild
void rebuildSwapchain(entt::registry& registry);  // fullscreen + swapchain resize combined

void shutdown(entt::registry& registry);
}  // namespace RenderLoop
