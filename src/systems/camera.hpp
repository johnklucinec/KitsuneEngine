#pragma once
#include <entt/entity/fwd.hpp>
#include <cstdint>

namespace sys {
// Reads : Input.mouse_delta, Camera.sensitivity
// Writes: Camera.yaw_rad, Camera.pitch_rad, Transform.rotation,
//         WorldMatrix via (DirtyCameraTransform)
//         ProjectionMatrix (via DirtyCameraProjection)
//
// Call AFTER sys::input() so mouse_delta is already accumulated.
// vp_width / vp_height — current swapchain/viewport dimensions.
void camera(entt::registry& reg, uint32_t vp_width, uint32_t vp_height);
}  // namespace sys
