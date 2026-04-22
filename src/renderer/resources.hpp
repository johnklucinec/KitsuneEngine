#pragma once

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>
#include "slang/slang.h"
#include "slang/slang-com-ptr.h"
#include <glm/glm.hpp>
#include <array>
#include "common.hpp"
#include <entt/entity/fwd.hpp>

struct VkContext;
struct FrameState;

struct ShaderData  // Look into scalarBlockLayout
{
  glm::mat4 projection;
  glm::mat4 view;
  glm::vec4 lightPos = { 0.0f, -10.0f, 10.0f, 0.0f };
};

struct InstanceData
{
  glm::mat4 model;
};

struct ShaderDataBuffer
{
  VmaAllocation     allocation = VK_NULL_HANDLE;
  VmaAllocationInfo allocationInfo{};
  VkBuffer          buffer        = VK_NULL_HANDLE;
  VkDeviceAddress   deviceAddress = 0;
};

struct Texture
{
  VmaAllocation allocation = VK_NULL_HANDLE;
  VkImage       image      = VK_NULL_HANDLE;
  VkImageView   view       = VK_NULL_HANDLE;
  VkSampler     sampler    = VK_NULL_HANDLE;
};

struct SceneResources
{
  VkBuffer                                           vBuffer      = VK_NULL_HANDLE;
  VmaAllocation                                      vBufferAlloc = VK_NULL_HANDLE;
  uint32_t                                           indexCount   = 0;
  VkDeviceSize                                       indexOffset  = 0;
  std::array<ShaderDataBuffer, MAX_FRAMES_IN_FLIGHT> shaderDataBuffers;
  std::array<ShaderDataBuffer, MAX_FRAMES_IN_FLIGHT> instanceBuffers;
  std::vector<Texture>                               textures;
  std::vector<VkDescriptorImageInfo>                 textureDescriptors;
  Slang::ComPtr<slang::IGlobalSession>               slangSession;
};

namespace Renderer {
void initSceneResources(entt::registry& registry);
void destroySceneResources(entt::registry& registry);
}  // namespace Renderer
