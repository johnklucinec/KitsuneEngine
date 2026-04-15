#pragma once
#include <entt/entt.hpp>

namespace sys {

// Emplaces FramePacerState into ctx(), acquires the OS timer, sets the
// scheduler period to its minimum, and elevates thread priority.
void frame_pacer_init(entt::registry& reg);

// Call after wait on fence
// Sleeps until the next frame deadline, then writes deltaTime / currentFps / deltaUs
void frame_pacer(entt::registry& reg);

void frame_pacer_shutdown(entt::registry& reg);

}  // namespace sys
