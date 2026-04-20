#include "frame.hpp"
#include "context.hpp"
#include "common.hpp"
#include "swapchain.hpp"
#include "entt/entity/registry.hpp"

namespace Renderer {

void syncFrameSemaphores(entt::registry& registry)
{
  auto&       fs  = registry.ctx().get<FrameState>();
  const auto& ctx = registry.ctx().get<VkContext>();
  const auto& sc  = registry.ctx().get<SwapchainState>();

  VkSemaphoreCreateInfo semaphoreCI{
    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
  };

  for(auto& sem : fs.renderSemaphores)
    vkDestroySemaphore(ctx.device, sem, nullptr);

  fs.renderSemaphores.resize(sc.images.size());
  for(auto& sem : fs.renderSemaphores)
    chk(vkCreateSemaphore(ctx.device, &semaphoreCI, nullptr, &sem));
}

void initFrameState(entt::registry& registry)
{
  auto&       fs  = registry.ctx().get<FrameState>();
  const auto& ctx = registry.ctx().get<VkContext>();
  const auto& sc  = registry.ctx().get<SwapchainState>();

  // ========================================
  // Synchronization Objects
  VkSemaphoreCreateInfo semaphoreCI{
    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
  };
  VkFenceCreateInfo fenceCI{
    .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    .flags = VK_FENCE_CREATE_SIGNALED_BIT,  // start signaled so frame 0 doesn't block
  };

  // Allocate all per-frame command buffers in one call
  std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> cbs{};

  VkCommandBufferAllocateInfo cbAllocCI{
    .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .commandPool        = ctx.commandPool,
    .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    .commandBufferCount = fs.framesInFlight,
  };
  chk(vkAllocateCommandBuffers(ctx.device, &cbAllocCI, cbs.data()));

  // One Per-frame-in-flight: fence + present semaphore + command buffer
  for(uint32_t i = 0; i < fs.framesInFlight; ++i)
  {
    chk(vkCreateFence(ctx.device, &fenceCI, nullptr, &fs.frames[i].fence));
    chk(vkCreateSemaphore(ctx.device, &semaphoreCI, nullptr, &fs.frames[i].presentSem));
    fs.frames[i].commandBuffer = cbs[i];
  }

  // Per-swapchain-image: one render semaphore each
  syncFrameSemaphores(registry);
}

void destroyFrameState(entt::registry& registry)
{
  auto&       fs  = registry.ctx().get<FrameState>();
  const auto& ctx = registry.ctx().get<VkContext>();
  const auto& sc  = registry.ctx().get<SwapchainState>();

  // Collect and free command buffers explicitly before pool teardown
  std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> cbs{};
  for(uint32_t i = 0; i < fs.framesInFlight; ++i)
    cbs[i] = fs.frames[i].commandBuffer;
  vkFreeCommandBuffers(ctx.device, ctx.commandPool, fs.framesInFlight, cbs.data());

  for(uint32_t i = 0; i < fs.framesInFlight; ++i)
  {
    vkDestroyFence(ctx.device, fs.frames[i].fence, nullptr);
    vkDestroySemaphore(ctx.device, fs.frames[i].presentSem, nullptr);
  }

  for(auto& sem : fs.renderSemaphores)
    vkDestroySemaphore(ctx.device, sem, nullptr);
  fs.renderSemaphores.clear();
}

}  // namespace Renderer
