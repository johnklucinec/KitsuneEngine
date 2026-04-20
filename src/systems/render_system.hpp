#pragma once
#include <entt/entity/fwd.hpp>

namespace System {

// Initializes the render system and runs the render loop.
void render(entt::registry& registry);

}  // namespace System
