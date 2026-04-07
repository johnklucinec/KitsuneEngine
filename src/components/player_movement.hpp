#pragma once

inline constexpr float PC_BASE_SPEED     = 5.5f;
inline constexpr float PC_SPRINT_MULT    = 1.5f;
inline constexpr float PC_BACKWARD_MULT  = 0.90f;
inline constexpr float PC_BACK_DIAG_MULT = 0.933333f;
inline constexpr float PC_AXIS_EPS       = 1e-3f;

struct PlayerMovement
{
  float move_speed{ PC_BASE_SPEED };
};
