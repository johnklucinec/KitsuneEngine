#pragma once
#include <entt/entt.hpp>

namespace sys {

// Call ONCE at startup (before the main loop).
// Emplaces FramePacerState into ctx(), acquires the OS timer, sets the
// scheduler period to its minimum, and elevates thread priority.
void frame_pacer_init(entt::registry& reg);

// Call at the TOP of each loop iteration, before input + render.
// Sleeps until the next frame deadline, then writes deltaTime / currentFps /
// deltaUs back into ctx().get<FramePacerState>().
void frame_pacer(entt::registry& reg);

// Call ONCE at shutdown to release OS resources.
void frame_pacer_shutdown(entt::registry& reg);

}  // namespace sys
