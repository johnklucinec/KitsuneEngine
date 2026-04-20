#pragma once
#include <glm/glm.hpp>

struct Camera
{
  float  fov{ 103.0f };
  float  z_near{ 0.1f };
  float  z_far{ 1000.0f };
  double yaw_rad{ 0.0 };
  double pitch_rad{ 0.0 };
  float  sensitivity{ 2.70f };
};

struct ProjectionMatrix
{
  glm::mat4 matrix{ 1.f };  // computed by camera, read by render
};
