#pragma once
#include <vulkan/vulkan.h>

struct VkContext;
struct SwapchainState;
struct SceneResources;

struct PipelineState
{
  VkPipeline            pipeline      = VK_NULL_HANDLE;
  VkPipelineLayout      layout        = VK_NULL_HANDLE;
  VkDescriptorSetLayout descSetLayout = VK_NULL_HANDLE;
  VkDescriptorPool      descPool      = VK_NULL_HANDLE;
  VkDescriptorSet       descSet       = VK_NULL_HANDLE;
};

namespace renderer {
void initPipeline(PipelineState& ps, const VkContext& ctx, const SwapchainState& sc, const SceneResources& res);
void destroyPipeline(PipelineState& ps, const VkContext& ctx);
}  // namespace renderer
