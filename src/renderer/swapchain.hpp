#pragma once
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>
#include <vector>
#include <SDL3/SDL_video.h>
#include "renderer/context.hpp"

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

namespace renderer {
void initSwapchain(SwapchainState& sc, const VkContext& ctx, SDL_Window* window, bool fullscreen);
void destroySwapchain(SwapchainState& sc, const VkContext& ctx);
void rebuildSwapchain(SwapchainState& sc, const VkContext& ctx);
}  // namespace renderer
