#pragma once
#include <entt/entity/fwd.hpp>

namespace System {

// Emplaces FramePacerState into ctx(), acquires the OS timer, sets the
// scheduler period to its minimum, and elevates thread priority.
void framePacerInit(entt::registry& reg);

// Call after wait on fence
// Sleeps until the next frame deadline, then writes deltaTime / currentFps / deltaUs
void framePacer(entt::registry& reg);

void framePacerShutdown(entt::registry& reg);

}  // namespace System
