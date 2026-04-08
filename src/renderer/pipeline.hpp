#pragma once
#include <vulkan/vulkan.h>
#include <span>

struct VkContext;

struct PipelineState
{
  VkPipeline            pipeline      = VK_NULL_HANDLE;
  VkPipelineLayout      layout        = VK_NULL_HANDLE;
  VkDescriptorSetLayout descSetLayout = VK_NULL_HANDLE;
  VkDescriptorPool      descPool      = VK_NULL_HANDLE;
  VkDescriptorSet       descSet       = VK_NULL_HANDLE;
};

namespace renderer {
void initPipeline(PipelineState& ps, const VkContext& ctx, VkFormat colorFmt, VkFormat depthFmt, std::span<const VkDescriptorImageInfo> textures);
void destroyPipeline(PipelineState& ps, const VkContext& ctx);
}  // namespace renderer
