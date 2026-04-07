#pragma once
#include <entt/entt.hpp>

namespace sys {
// Reads : Input (WASD / LShift), Camera.yaw_rad, PlayerController.move_speed
// Writes: Velocity.linear  →  Transform.position
//
// Call AFTER sys::input() and sys::camera() so yaw and keys are current.
void player_movement(entt::registry& reg, float dt);
}  // namespace sys
