#pragma once
#include <entt/entity/fwd.hpp>

namespace System {

// Controls the application state (e.g. window close, fullscreen)
// Call at the end of the game loop
void appControls(entt::registry& reg);

}  // namespace System
