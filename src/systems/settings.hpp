#pragma once
#include <entt/entity/fwd.hpp>

namespace sys {

// Emplaces settings into the registry
void settings_init(entt::registry& reg, int argc, char* argv[]);

// Call at the end of the game loop
void settings_loop(entt::registry& reg);
}  // namespace sys
