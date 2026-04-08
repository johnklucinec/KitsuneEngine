#pragma once

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>
#include "slang/slang.h"
#include "slang/slang-com-ptr.h"
#include <glm/glm.hpp>
#include <array>
#include "common.hpp"

struct VkContext;

struct ShaderData  // Look into scalarBlockLayout
{
  glm::mat4 projection;
  glm::mat4 view;
  glm::mat4 model[3];
  glm::vec4 lightPos = { 0.0f, -10.0f, 10.0f, 0.0f };
  uint32_t  selected = 1;
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
  std::array<Texture, 3>                             textures{};
  Slang::ComPtr<slang::IGlobalSession>               slangSession;
};

namespace renderer {
// Returns texture descriptors needed by initPipeline
std::vector<VkDescriptorImageInfo> initSceneResources(SceneResources& res, const VkContext& ctx, uint32_t framesInFlight);
void                               destroySceneResources(SceneResources& res, const VkContext& ctx);
}  // namespace renderer
