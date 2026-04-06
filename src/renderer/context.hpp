#pragma once
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

struct VkContext
{
  VkInstance    instance    = VK_NULL_HANDLE;
  VkDevice      device      = VK_NULL_HANDLE;
  VkQueue       queue       = VK_NULL_HANDLE;
  VkSurfaceKHR  surface     = VK_NULL_HANDLE;
  VkCommandPool commandPool = VK_NULL_HANDLE;
  VmaAllocator  allocator   = VK_NULL_HANDLE;
};
