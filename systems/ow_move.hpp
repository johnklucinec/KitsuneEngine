#pragma once
#include <cmath>

namespace ow_move {

// ========================================
// Constants (Overwatch-like values, m/s)
inline constexpr float BASE_SPEED     = 5.5f;
inline constexpr float SPRINT_MULT    = 1.5f;
inline constexpr float BACKWARD_MULT  = 0.90f;      // straight back
inline constexpr float BACK_DIAG_MULT = 0.933333f;  // back + strafe
inline constexpr float AXIS_EPS       = 1e-3f;

// ========================================
// 2D vector: local X (right), Y (forward)
struct Vec2
{
  float x = 0.0f;
  float y = 0.0f;

  [[nodiscard]]
  constexpr float length_sq() const noexcept
  {
    return x * x + y * y;
  }

  constexpr void normalize_safe() noexcept
  {
    float lsq = length_sq();
    if(lsq > 0.0f)
    {
      float inv_len = 1.0f / std::sqrt(lsq);
      x *= inv_len;
      y *= inv_len;
    }
  }
};

// 3D vector: world X (right), Y (up), Z (forward)
struct Vec3
{
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;
};

// ========================================
struct Walker
{
  // Local-space horizontal velocity (right, forward), in m/s.
  Vec2 vel_local{};

  // Feed raw axis input each frame. Velocity is set instantly (no ramp-up).
  constexpr void update(float fwd, float side, bool sprint) noexcept
  {
    bool has_fwd   = (fwd > AXIS_EPS);
    bool has_back  = (fwd < -AXIS_EPS);
    bool has_right = (side > AXIS_EPS);
    bool has_left  = (side < -AXIS_EPS);

    Vec2 wish{ side, fwd };

    if(wish.length_sq() <= 0.0f)
    {
      vel_local = Vec2{};
      return;
    }

    wish.normalize_safe();

    // Directional speed penalty (matches OW backward movement).
    float dir_mult = 1.0f;
    if(has_back && !has_left && !has_right)
      dir_mult = BACKWARD_MULT;
    else if(has_back && (has_left || has_right))
      dir_mult = BACK_DIAG_MULT;

    float speed = BASE_SPEED * dir_mult;

    if(sprint && has_fwd)
      speed *= SPRINT_MULT;

    vel_local.x = wish.x * speed;
    vel_local.y = wish.y * speed;
  }

  // Advance a local-space 2D position by one timestep.
  [[nodiscard]]
  constexpr Vec2 integrate(const Vec2& pos, float dt) const noexcept
  {
    return Vec2{ pos.x + vel_local.x * dt, pos.y + vel_local.y * dt };
  }

  // Rotate local velocity into world space and advance a world-space position.
  // yaw_rad is a Y-axis rotation (camera/entity facing direction).
  constexpr void integrate_world(float& x, float& z, float yaw_rad, float dt) const noexcept
  {
    const float cy = std::cos(yaw_rad);
    const float sy = std::sin(yaw_rad);
    x += (vel_local.x * cy - vel_local.y * sy) * dt;
    z += (vel_local.x * sy + vel_local.y * cy) * dt;
  }
};

}  // namespace ow_move
