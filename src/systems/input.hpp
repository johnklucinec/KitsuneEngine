#pragma once
#include <entt/entity/fwd.hpp>

namespace sys {

// Polls all pending OS events and writes raw state into every Input component.
void input(entt::registry& reg);

}  // namespace sys
