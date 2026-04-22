#pragma once
#include "types.hpp"
#include <string_view>
#include <vector>
#include <optional>
#include <source_location>

namespace MeshLoader {

struct MeshData
{
  std::vector<Vertex>   vertices;
  std::vector<uint16_t> indices;
};

// Returns nullopt on failure
[[nodiscard]] std::optional<MeshData> load(std::string_view path, std::source_location loc = std::source_location::current());

}  // namespace MeshLoader
