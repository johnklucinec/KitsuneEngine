#pragma once
#include <cmath>
#include <numbers>
#include <algorithm>
#include <glm/glm.hpp>

namespace ow_cam {

// OW2 mouse-to-angle scale at sensitivity 1 (degrees per raw count)
inline constexpr float M_YAW      = 0.006666f;
inline constexpr float M_PITCH    = 0.006666f;
inline constexpr float REF_ASPECT = 16.0f / 9.0f;
inline constexpr float DEG_TO_RAD = std::numbers::pi_v<float> / 180.0f;

// ========================================
// FOV helpers

// Convert horizontal FOV (degrees, at 16:9) → vertical FOV (radians).
// Pass the result to glm::perspective. Call once when the FOV slider changes.
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
  glm::vec3 pos         = { 0.0f, 0.0f, 0.0f };
  double    yaw_rad     = 0.0f;
  double    pitch_rad   = 0.0f;
  float     sensitivity = 2.70f;  // OW2 slider range (0.01–100)

  // Feed raw mouse delta counts each input event.
  constexpr void apply_mouse_delta(int dx, int dy) noexcept
  {
    constexpr double MY = M_YAW * std::numbers::pi / 180.0;
    constexpr double MP = M_PITCH * std::numbers::pi / 180.0;
    yaw_rad += static_cast<double>(dx) * MY * sensitivity;
    pitch_rad += static_cast<double>(dy) * MP * sensitivity;

    constexpr double PITCH_LIMIT = std::numbers::pi * 0.499;
    pitch_rad                    = std::clamp(pitch_rad, -PITCH_LIMIT, PITCH_LIMIT);
  }

  // Call when building view matrix
  [[nodiscard]] float yaw_f() const noexcept { return static_cast<float>(yaw_rad); }
  [[nodiscard]] float pitch_f() const noexcept { return static_cast<float>(pitch_rad); }
};

// ========================================
// cm/360 utilities  —  K = 2.54 * 360 / M_YAW ≈ 137,160

inline constexpr float K = 2.54f * 360.0f / M_YAW;

[[nodiscard]]
constexpr float cm_per_360(float dpi, float sens) noexcept
{
  return K / (dpi * sens);
}

[[nodiscard]]
constexpr float sens_from_cm360(float dpi, float target_cm360) noexcept
{
  return K / (dpi * target_cm360);
}

}  // namespace ow_cam
