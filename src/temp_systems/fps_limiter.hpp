// fps_limiter.hpp
#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <chrono>
#include <thread>
#include <immintrin.h>

#pragma comment(lib, "winmm.lib")

namespace fps {

using Clock     = std::chrono::high_resolution_clock;
using TimePoint = Clock::time_point;

namespace detail {

constexpr int64_t TOLERANCE_NS      = 1'020'000LL;  // ~1ms
inline int        schedulerPeriodMs = 1;            // Used to cap individual sleep slices

// ========================================
// Sleeps the calling thread until [target], using a high-resolution waitable timer followed by a spin-wait for sub-ms precision.
inline void sleepUntil(TimePoint target)
{
  // High-resolution waitable timer; falls back to sleep_for if unavailable
  static HANDLE timer = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);

  // Query the hardware's actual minimum timer resolution and set it
  static bool init = [] {
    TIMECAPS caps;
    timeGetDevCaps(&caps, sizeof caps);
    timeBeginPeriod(caps.wPeriodMin);
    schedulerPeriodMs = static_cast<int>(caps.wPeriodMin);
    return true;
  }();
  (void)init;

  if(timer)
  {
    // Cap each sleep slice to just under one scheduler period so a single late OS wakeup
    // can only contribute ~1 period of error before we re-evaluate remaining time
    const int64_t maxTicks = static_cast<int64_t>(schedulerPeriodMs) * 9'500;

    for(;;)
    {
      int64_t remaining = (target - Clock::now()).count();
      int64_t ticks     = (remaining - TOLERANCE_NS) / 100;
      if(ticks <= 0)
        break;
      LARGE_INTEGER due{ .QuadPart = -(ticks > maxTicks ? maxTicks : ticks) };  // negative = relative time
      SetWaitableTimerEx(timer, &due, 0, nullptr, nullptr, nullptr, 0);
      WaitForSingleObject(timer, INFINITE);
    }
  }
  else
  {
    // Fallback: sleep slightly short of the target to leave room for the spin-wait below
    double ms = std::chrono::duration<double, std::milli>(target - Clock::now()).count() - static_cast<double>(TOLERANCE_NS) * 1e-6;
    if(ms > 0)
      std::this_thread::sleep_for(std::chrono::duration<double, std::milli>(ms));
  }

  // Spin the remaining sub-ms gap for precise wakeup
  while(Clock::now() < target)
    _mm_pause();
}

}  // namespace detail

// ========================================
// // Frame-rate limiter; construct once per thread, then call .wait() at the top of your render loop.
struct Limiter
{
  explicit Limiter(double fps) // setting fps to zero disables limiter.
      : frameDuration(toDuration(fps))
      , _deltaUs(fps > 0.0 ? static_cast<int64_t>(1e6 / fps) : 16667LL)
  {
    auto now = Clock::now();
    deadline = now + frameDuration;
    lastWake = now;
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
  }

  // Call at the TOP of your loop, before input + render.
  void wait()
  {
    if(frameDuration != Clock::duration::zero())
      detail::sleepUntil(deadline);

    auto now = Clock::now();
    _deltaUs = std::chrono::duration_cast<std::chrono::microseconds>(now - lastWake).count();
    lastWake = now;

    if(frameDuration != Clock::duration::zero())
    {
      deadline += frameDuration;
      if(deadline < now - frameDuration * 4)  // If we're more than 4 frames behind (e.g. debugger pause, system stall), reset instead of catching up
        deadline = now + frameDuration;
    }
  }

  void setFPS(double fps) { frameDuration = toDuration(fps); }

  // ========================================
  // Read-only frame timing accessors; safe to call immediately after wait().
  [[nodiscard]] float   deltaTime() const { return static_cast<float>(_deltaUs) * 1e-6f; }
  [[nodiscard]] float   currentFPS() const { return _deltaUs > 0 ? 1e6f / static_cast<float>(_deltaUs) : 0.0f; }
  [[nodiscard]] int64_t deltaUs() const { return _deltaUs; }


private:
  // Converts a frames-per-second value to the equivalent std::chrono duration.
  static Clock::duration toDuration(double fps)
  {
    if(fps <= 0.0)
      return Clock::duration::zero();
    return std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(1.0 / fps));
  }

  Clock::duration frameDuration;
  TimePoint       deadline;
  TimePoint       lastWake;
  int64_t         _deltaUs;
};

}  // namespace fps
