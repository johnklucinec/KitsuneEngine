#pragma once
#include "frame_pacer.hpp"

namespace PacerUtils {

// Converts fps to a chrono duration. fps <= 0 returns zero (= unlimited).
[[nodiscard]] FramePacerState::Duration pacerToDuration(double fps);

// Blocks the calling thread until [target].
// Uses the high-resolution waitable timer in [state], then spin-waits the
// remaining sub-ms gap for precise wakeup.
// Falls back to sleep_for if timerHandle is null.
void pacerSleepUntil(FramePacerState::TimePoint target, const FramePacerState& state);

}  // namespace PacerUtils
