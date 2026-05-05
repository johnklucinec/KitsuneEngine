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
  uint32_t  textureIndex;  // which slot in res.textureDescriptors to sample
  uint32_t  _pad[3];       // keep 16-byte alignment for the SSBO
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

struct MeshResource
{
  VkBuffer      buffer      = VK_NULL_HANDLE;
  VmaAllocation allocation  = VK_NULL_HANDLE;
  VkDeviceSize  indexOffset = 0;
  uint32_t      indexCount  = 0;
};

struct SceneResources
{
  std::vector<MeshResource>                          meshes;
  std::array<ShaderDataBuffer, MAX_FRAMES_IN_FLIGHT> shaderDataBuffers;
  std::array<ShaderDataBuffer, MAX_FRAMES_IN_FLIGHT> instanceBuffers;
  std::vector<Texture>                               textures;
  std::vector<VkDescriptorImageInfo>                 textureDescriptors;
  Slang::ComPtr<slang::IGlobalSession>               slangSession;
  std::unordered_map<std::string, uint32_t>          meshCache;
  std::unordered_map<std::string, uint32_t>          textureCache;
};

namespace Renderer {
void     initSceneResources(entt::registry& registry);
uint32_t loadMesh(const std::string& path, entt::registry& registry);
uint32_t loadTexture(const std::string& path, entt::registry& registry);
void     updateDescriptorSets(entt::registry& registry);
void     destroySceneResources(entt::registry& registry);
}  // namespace Renderer
