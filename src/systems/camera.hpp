#pragma once
#include <entt/entt.hpp>

namespace sys {
// Reads : Input.mouse_delta, Camera.sensitivity
// Writes: Camera.yaw_rad, Camera.pitch_rad, Transform.rotation
//
// Call AFTER sys::input() so mouse_delta is already accumulated.
void camera(entt::registry& reg);
}  // namespace sys
