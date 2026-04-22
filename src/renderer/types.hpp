#pragma once
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float2.hpp>

using VkDeviceAddress = uint64_t;

// ========================================
// Structs and Buffers
struct Vertex
{
  glm::vec3 pos;
  glm::vec3 normal;
  glm::vec2 uv;
};

struct PushConstants
{
  VkDeviceAddress shaderData;
  VkDeviceAddress instances;
};
