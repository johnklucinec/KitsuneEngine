#pragma once
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <slang.h>
#include <slang-com-ptr.h>
#include <glm/glm.hpp>
#include <array>

constexpr uint32_t MAX_FRAMES_IN_FLIGHT{ 2 };

struct Vertex
{
  glm::vec3 pos;
  glm::vec3 normal;
  glm::vec2 uv;
};

struct ShaderData
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
  ShaderData                                         shaderData{};
  std::array<ShaderDataBuffer, MAX_FRAMES_IN_FLIGHT> shaderDataBuffers;
  std::array<Texture, 3>                             textures{};
  Slang::ComPtr<slang::IGlobalSession>               slangSession;
};
