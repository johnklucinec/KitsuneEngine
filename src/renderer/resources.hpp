#pragma once

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>
#include "slang/slang.h"
#include "slang/slang-com-ptr.h"
#include <glm/glm.hpp>
#include <array>
#include "common.hpp"
#include <entt/entity/fwd.hpp>

#include "../assets/shaders/shader_constants.h"  // TODO: TEMPORARY IDEA

struct VkContext;
struct FrameState;

struct ShaderData  // Look into scalarBlockLayout
{
  glm::mat4 projection;
  glm::mat4 view;
  glm::mat4 model[INSTANCE_COUNT];
  glm::vec4 lightPos = { 0.0f, -10.0f, 10.0f, 0.0f };
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
  ShaderData                                         shaderData{};
  std::array<ShaderDataBuffer, MAX_FRAMES_IN_FLIGHT> shaderDataBuffers;
  std::array<Texture, INSTANCE_COUNT>                textures{};
  std::vector<VkDescriptorImageInfo>                 textureDescriptors;
  Slang::ComPtr<slang::IGlobalSession>               slangSession;
};

namespace Renderer {
void initSceneResources(entt::registry& registry);
void destroySceneResources(entt::registry& registry);
}  // namespace Renderer
