#pragma once
#include <glm/glm.hpp>

struct Velocity
{
  glm::vec3 linear{ 0.0f };  // world-space m/s; Y is unused for ground movement
};
