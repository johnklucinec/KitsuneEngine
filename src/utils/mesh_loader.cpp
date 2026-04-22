#include "mesh_loader.hpp"
#include "log_utils.hpp"
#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <filesystem>

namespace MeshLoader {

std::optional<MeshData> load(std::string_view path, std::source_location loc)
{
  fastgltf::Parser parser;
  MeshData         out;

  // 1. Load Buffer Data
  auto data = fastgltf::GltfDataBuffer::FromPath(path);
  if(data.error() != fastgltf::Error::None)
  {
    Log::error("MeshLoader Error", "Failed to read file: " + std::string(path), loc);
    return std::nullopt;
  }

  // 2. Parse glTF
  auto asset = parser.loadGltf(data.get(), std::filesystem::path(path).parent_path(), fastgltf::Options::LoadExternalBuffers);
  if(asset.error() != fastgltf::Error::None)
  {
    Log::error("MeshLoader Error", "Parse error in " + std::string(path) + ": " + std::string(fastgltf::getErrorMessage(asset.error())), loc);
    return std::nullopt;
  }

  fastgltf::Asset& gltf = asset.get();

  // 3. Process Meshes/Primitives
  for(auto& mesh : gltf.meshes)
  {
    for(auto& prim : mesh.primitives)
    {
      const size_t base_vtx = out.vertices.size();

      // Position is mandatory for our MeshData
      auto* posAttr = prim.findAttribute("POSITION");
      if(posAttr == prim.attributes.end())
        continue;

      auto& posAcc = gltf.accessors[posAttr->accessorIndex];
      out.vertices.resize(base_vtx + posAcc.count);

      // Extract Positions
      fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, posAcc, [&](glm::vec3 v, size_t i) { out.vertices[base_vtx + i].pos = v; });

      // Extract Indices
      if(prim.indicesAccessor.has_value())
      {
        auto& idxAcc = gltf.accessors[*prim.indicesAccessor];
        out.indices.reserve(out.indices.size() + idxAcc.count);
        fastgltf::iterateAccessor<uint32_t>(gltf, idxAcc, [&](uint32_t idx) { out.indices.push_back(static_cast<uint16_t>(base_vtx + idx)); });
      }

      // Extract Normals & UVs
      auto loadAttr = [&](const char* name, auto& field) {
        if(auto* attr = prim.findAttribute(name); attr != prim.attributes.end())
        {
          fastgltf::iterateAccessorWithIndex<decltype(field)>(gltf, gltf.accessors[attr->accessorIndex],
                                                              [&](auto v, size_t i) { out.vertices[base_vtx + i].*field = v; });
        }
      };

      if(auto* n = prim.findAttribute("NORMAL"))
        fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, gltf.accessors[n->accessorIndex],
                                                      [&](glm::vec3 v, size_t i) { out.vertices[base_vtx + i].normal = v; });

      if(auto* t = prim.findAttribute("TEXCOORD_0"))
        fastgltf::iterateAccessorWithIndex<glm::vec2>(gltf, gltf.accessors[t->accessorIndex], [&](glm::vec2 v, size_t i) { out.vertices[base_vtx + i].uv = v; });
    }
  }

  if(out.vertices.empty())
  {
    Log::error("MeshLoader Error", "Mesh loaded but contained no geometry: " + std::string(path), loc);
    return std::nullopt;
  }

  return out;
}

}  // namespace MeshLoader
