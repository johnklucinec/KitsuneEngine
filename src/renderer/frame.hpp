#pragma once
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>
#include <array>
#include <vector>
#include "common.hpp"
#include <entt/entity/fwd.hpp>

struct VkContext;
struct SwapchainState;

struct FrameData
{
  VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
  VkFence         fence         = VK_NULL_HANDLE;
  VkSemaphore     presentSem    = VK_NULL_HANDLE;
};

struct FrameState
{
  uint32_t                                    frameIndex     = 0;
  uint32_t                                    framesInFlight = MAX_FRAMES_IN_FLIGHT;
  std::array<FrameData, MAX_FRAMES_IN_FLIGHT> frames;
  std::vector<VkSemaphore>                    renderSemaphores;  // one per swapchain image

  FrameData&       currentFrame() { return frames[frameIndex]; }
  const FrameData& currentFrame() const { return frames[frameIndex]; }
};

namespace Renderer {
void initFrameState(entt::registry& registry);
void syncFrameSemaphores(entt::registry& registry);
void destroyFrameState(entt::registry& registry);
}  // namespace Renderer
