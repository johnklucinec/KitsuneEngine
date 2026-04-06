#pragma once
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <vector>

struct SwapchainState
{
  VkSwapchainKHR           swapchain = VK_NULL_HANDLE;
  std::vector<VkImage>     images;
  std::vector<VkImageView> imageViews;
  VkImage                  depthImage      = VK_NULL_HANDLE;
  VkImageView              depthImageView  = VK_NULL_HANDLE;
  VmaAllocation            depthAlloc      = VK_NULL_HANDLE;
  uint32_t                 imageIndex      = 0;
  bool                     needsRebuild    = false;
  bool                     needsFullscreen = true;
};
