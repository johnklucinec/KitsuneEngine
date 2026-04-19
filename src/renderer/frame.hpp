#pragma once
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>
#include <array>
#include <vector>
#include "common.hpp"

struct VkContext;
struct SwapchainState;

struct FrameData
{
  VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
  VkFence         fence         = VK_NULL_HANDLE;
  VkSemaphore     presentSem    = VK_NULL_HANDLE;
  // ShaderDataBuffer lives here too — see resources.hpp
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

namespace renderer {
void initFrameState(FrameState& fs, const VkContext& ctx, const SwapchainState& sc);
void syncFrameSemaphores(FrameState& fs, const VkContext& ctx, const SwapchainState& sc);
void destroyFrameState(FrameState& fs, const VkContext& ctx);
}  // namespace renderer
