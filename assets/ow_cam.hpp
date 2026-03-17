#pragma once
#include <cmath>
#include <numbers>
#include <algorithm>

namespace ow_cam {

// ========================================
// Constants (OW2 engine values)
inline constexpr float M_YAW      = 0.006666f;  // deg/count @sens1
inline constexpr float M_PITCH    = 0.006666f;
inline constexpr float REF_ASPECT = 16.0f / 9.0f;
inline constexpr float DEG_TO_RAD = std::numbers::pi_v<float> / 180.0f;

// ========================================
// FOV helpers

// Call once when the FOV slider changes.
// fov_cfg_deg: horizontal FOV in degrees, measured at 16:9 (e.g. 103)
// Returns vertical FOV in radians — pass this to glm::perspective.
[[nodiscard]]
constexpr float vfov_from_hfov_cfg(float fov_cfg_deg) noexcept
{
  float h_rad = fov_cfg_deg * DEG_TO_RAD;
  return 2.0f * std::atan(std::tan(h_rad * 0.5f) / REF_ASPECT);
}

// ========================================
// Camera state
struct Camera
{
  float yaw_rad     = 0.0f;
  float pitch_rad   = 0.0f;
  float sensitivity = 2.70f;  // OW2 in-game slider (0.01–100)

  // Call with raw mouse delta counts each input event.
  constexpr void apply_mouse_delta(int dx, int dy) noexcept
  {
    yaw_rad += static_cast<float>(dx) * M_YAW * sensitivity * DEG_TO_RAD;
    pitch_rad += static_cast<float>(dy) * M_PITCH * sensitivity * DEG_TO_RAD;

    // Clamp pitch to avoid gimbal flip
    constexpr float PITCH_LIMIT = std::numbers::pi_v<float> * 0.499f;
    pitch_rad                   = std::clamp(pitch_rad, -PITCH_LIMIT, PITCH_LIMIT);
  }
};

// ========================================
// cm/360 utilities

// K = 2.54 * 360 / M_YAW  ≈ 137,160
inline constexpr float K = 2.54f * 360.0f / M_YAW;

// Compute cm/360 for a given DPI and sensitivity.
[[nodiscard]]
constexpr float cm_per_360(float dpi, float sens) noexcept
{
  return K / (dpi * sens);
}

// Invert: get sensitivity that matches a desired cm/360 at given DPI.
[[nodiscard]]
constexpr float sens_from_cm360(float dpi, float target_cm360) noexcept
{
  return K / (dpi * target_cm360);
}

}  // namespace ow_cam
