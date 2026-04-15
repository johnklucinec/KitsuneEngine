#pragma once
#include <entt/entt.hpp>

namespace sys {
// Applies keyboard input to camera position
// Call AFTER sys::input() and sys::camera() so yaw and keys are current.
void player_movement(entt::registry& reg);
}  // namespace sys
