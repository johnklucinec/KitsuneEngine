#pragma once
#include <entt/entity/fwd.hpp>
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>
#include <vector>
#include <SDL3/SDL_video.h>

struct SwapchainState
{
  VkSwapchainKHR           swapchain = VK_NULL_HANDLE;
  std::vector<VkImage>     images;
  std::vector<VkImageView> imageViews;
  VkImage                  depthImage     = VK_NULL_HANDLE;
  VkImageView              depthImageView = VK_NULL_HANDLE;
  VmaAllocation            depthAlloc     = VK_NULL_HANDLE;
  VkExtent2D               extent{};
  VkFormat                 colorFormat     = VK_FORMAT_UNDEFINED;
  VkFormat                 depthFormat     = VK_FORMAT_UNDEFINED;
  uint32_t                 imageIndex      = 0;
  bool                     needsRebuild    = false;
  bool                     needsFullscreen = true;
  VkSwapchainCreateInfoKHR swapchainCI{};
  VkImageCreateInfo        depthImageCI{};
};

namespace Renderer {
void initSwapchain(entt::registry& registry);
void destroySwapchain(entt::registry& registry);
void rebuildSwapchain(entt::registry& registry);
}  // namespace Renderer
