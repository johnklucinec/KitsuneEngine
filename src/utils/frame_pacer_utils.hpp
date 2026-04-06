#pragma once
#include "../core/frame_pacer_state.hpp"

namespace utils {

// Converts fps to a chrono duration. fps <= 0 returns zero (= unlimited).
[[nodiscard]] FramePacerState::Duration fp_to_duration(double fps);

// Blocks the calling thread until [target].
// Uses the high-resolution waitable timer stored in [state], then spin-waits the remaining sub-ms gap for precise wakeup.
// Falls back to sleep_for if timerHandle is null.
void fp_sleep_until(FramePacerState::TimePoint target, const FramePacerState& state);

}  // namespace utils
