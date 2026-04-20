#pragma once
#include <entt/entity/fwd.hpp>

namespace System {

// Polls all pending OS events and writes raw state into every Input component.
void input(entt::registry& reg);

}  // namespace System
