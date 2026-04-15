#pragma once
#include "components/frame_pacer.hpp"

namespace utils {

// Converts fps to a chrono duration. fps <= 0 returns zero (= unlimited).
[[nodiscard]] FramePacerState::Duration pacer_to_duration(double fps);

// Blocks the calling thread until [target].
// Uses the high-resolution waitable timer in [state], then spin-waits the
// remaining sub-ms gap for precise wakeup.
// Falls back to sleep_for if timerHandle is null.
void pacer_sleep_until(FramePacerState::TimePoint target, const FramePacerState& state);

}  // namespace utils
