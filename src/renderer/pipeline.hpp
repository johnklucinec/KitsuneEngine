#pragma once
#include <vulkan/vulkan.h>

struct PipelineState
{
  VkPipeline            pipeline      = VK_NULL_HANDLE;
  VkPipelineLayout      layout        = VK_NULL_HANDLE;
  VkDescriptorSetLayout descSetLayout = VK_NULL_HANDLE;
  VkDescriptorPool      descPool      = VK_NULL_HANDLE;
  VkDescriptorSet       descSet       = VK_NULL_HANDLE;
};
