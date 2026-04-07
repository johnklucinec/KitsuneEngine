#pragma once
#include <numbers>
#include <glm/glm.hpp>

inline constexpr float CAM_M_YAW      = 0.006666f;
inline constexpr float CAM_M_PITCH    = 0.006666f;
inline constexpr float CAM_REF_ASPECT = 16.0f / 9.0f;

// Max pitch before gimbal / view flip (~89.8°)
inline constexpr double CAM_PITCH_LIMIT = std::numbers::pi * 0.499;

struct Camera
{
  float  fov{ 103.0f };
  float  z_near{ 0.1f };
  float  z_far{ 1000.0f };
  double yaw_rad{ 0.0 };
  double pitch_rad{ 0.0 };
  float  sensitivity{ 2.70f };  // OW2 slider range (0.01–100)
};
