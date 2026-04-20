#pragma once
#include <vulkan/vulkan.h>
#include "entt/entity/fwd.hpp"

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

namespace Renderer {
void initPipeline(entt::registry& registry);
void destroyPipeline(entt::registry& registry);
}  // namespace Renderer
