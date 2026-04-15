#pragma once
#include <entt/entity/fwd.hpp>

namespace sys {

// Applies mouse input to camera rotation and rebuilds the view matrix.
// Rebuilds the projection matrix if DirtyCameraProjection is present (resize/FOV).
void camera(entt::registry& reg);

}  // namespace sys
