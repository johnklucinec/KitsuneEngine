#pragma once
#include <entt/entity/fwd.hpp>

namespace System {
// Applies keyboard input to camera position
// Call AFTER System::input() and System::camera() so yaw and keys are current.
void playerMovement(entt::registry& reg);
}  // namespace System
