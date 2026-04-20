#include "frame_pacer_utils.hpp"
#include <immintrin.h>
#include <thread>

namespace PacerUtils {

// ~1ms of headroom before spin-locking: lets the OS timer fire slightly early
// without overshooting, with the spin loop covering the remaining gap precisely.
static constexpr int64_t SPIN_GUARD_NS = 1'020'000LL;

FramePacerState::Duration pacerToDuration(double fps)
{
  using namespace std::chrono;

  if(fps <= 0.0)
    return FramePacerState::Duration::zero();

  return duration_cast<FramePacerState::Duration>(duration<double>(1.0 / fps));
}

void pacerSleepUntil(FramePacerState::TimePoint target, const FramePacerState& state)
{
  using Clock = FramePacerState::Clock;

  if(state.timerHandle)
  {
    // Cap each sleep slice to just under one scheduler period so a single
    // late OS wakeup contributes at most ~1 period of error.
    const int64_t maxTicks = static_cast<int64_t>(state.schedulerPeriodMs) * 9'500;

    for(;;)
    {
      const int64_t remaining = (target - Clock::now()).count();
      const int64_t ticks     = (remaining - SPIN_GUARD_NS) / 100;
      if(ticks <= 0)
        break;
      LARGE_INTEGER due{ .QuadPart = -(ticks > maxTicks ? maxTicks : ticks) };
      SetWaitableTimerEx(state.timerHandle, &due, 0, nullptr, nullptr, nullptr, 0);
      WaitForSingleObject(state.timerHandle, INFINITE);
    }
  }
  else
  {
    // Fallback: sleep slightly short, leaving room for the spin below.
    const double ms = std::chrono::duration<double, std::milli>(target - Clock::now()).count() - static_cast<double>(SPIN_GUARD_NS) * 1e-6;

    if(ms > 0)
      std::this_thread::sleep_for(std::chrono::duration<double, std::milli>(ms));
  }

  // Spin the remaining sub-ms gap for precise wakeup
  while(Clock::now() < target)
    _mm_pause();
}

}  // namespace PacerUtils
