#pragma once
#include <cstdint>

// Index into SceneResources::meshes[]
struct MeshInstance
{
  uint32_t meshIndex = UINT32_MAX;  // points to res.meshes[meshIndex]
};

// Index into SceneResources::textures[]
struct TextureInstance
{
  uint32_t textureIndex = UINT32_MAX;  // points to res.textures[textureIndex]
};
