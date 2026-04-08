#pragma once
#include <SDL3/SDL_video.h>
#include <vma/vk_mem_alloc.h>
#include <volk/volk.h>
#include <SDL3/SDL_vulkan.h>

struct VkContext
{
  uint32_t         deviceIndex    = 0;
  uint32_t         queueFamily    = 0;
  VkInstance       instance       = VK_NULL_HANDLE;
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
  VkDevice         device         = VK_NULL_HANDLE;
  VkQueue          queue          = VK_NULL_HANDLE;
  VkSurfaceKHR     surface        = VK_NULL_HANDLE;
  VkCommandPool    commandPool    = VK_NULL_HANDLE;
  VmaAllocator     allocator      = VK_NULL_HANDLE;
};

namespace renderer {
void initContext(VkContext& ctx, SDL_Window* window);
void destroyContext(VkContext& ctx);
}  // namespace renderer
