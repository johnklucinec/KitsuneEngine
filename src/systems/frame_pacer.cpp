#include "frame_pacer.hpp"
#include "../core/frame_pacer_state.hpp"
#include "../core/settings.hpp"
#include "../utils/frame_pacer_utils.hpp"

#pragma comment(lib, "winmm.lib")
#include <mmsystem.h>

namespace sys {

void frame_pacer_init(entt::registry& reg)
{
  auto& state = reg.ctx().emplace<FramePacerState>();
  auto& cfg   = reg.ctx().get<Settings>();

  // Acquire a high-resolution waitable timer (Win10 1803+).
  state.timerHandle = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);

  // Lower the OS scheduler granularity to its hardware minimum.
  TIMECAPS caps{};
  timeGetDevCaps(&caps, sizeof caps);
  timeBeginPeriod(caps.wPeriodMin);
  state.schedulerPeriodMs = static_cast<int>(caps.wPeriodMin);

  // Elevate this thread so OS preemption doesn't introduce drift.
  SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

  // Seed timing from current settings.
  state.frameDuration = utils::fp_to_duration(static_cast<double>(cfg.fps_max));
  const auto now      = FramePacerState::Clock::now();
  state.deadline      = now + state.frameDuration;
  state.lastWake      = now;
}

void frame_pacer(entt::registry& reg)
{
  auto& state = reg.ctx().get<FramePacerState>();
  auto& cfg   = reg.ctx().get<Settings>();

  // Hot-reload: sync frame duration if fps_max changed at runtime.
  const auto wantedDuration = utils::fp_to_duration(static_cast<double>(cfg.fps_max));
  if(wantedDuration != state.frameDuration)
    state.frameDuration = wantedDuration;

  // Sleep until the next deadline (no-op when frameDuration == zero).
  if(state.frameDuration != FramePacerState::Duration::zero())
    utils::fp_sleep_until(state.deadline, state);

  // Capture actual wake-up time and update delta metrics.
  const auto now   = FramePacerState::Clock::now();
  state.deltaUs    = std::chrono::duration_cast<std::chrono::microseconds>(now - state.lastWake).count();
  state.deltaTime  = static_cast<float>(state.deltaUs) * 1e-6f;
  state.currentFps = state.deltaUs > 0 ? 1e6f / static_cast<float>(state.deltaUs) : 0.0f;
  state.lastWake   = now;

  // Advance deadline; reset if we're more than 4 frames behind
  // (e.g. after a debugger pause or system stall) to avoid a catch-up storm.
  if(state.frameDuration != FramePacerState::Duration::zero())
  {
    state.deadline += state.frameDuration;
    if(state.deadline < now - state.frameDuration * 4)
      state.deadline = now + state.frameDuration;
  }
}

void frame_pacer_shutdown(entt::registry& reg)
{
  auto& state = reg.ctx().get<FramePacerState>();

  if(state.timerHandle)
  {
    CloseHandle(state.timerHandle);
    state.timerHandle = nullptr;
  }

  // Restore default scheduler period.
  TIMECAPS caps{};
  timeGetDevCaps(&caps, sizeof caps);
  timeEndPeriod(caps.wPeriodMin);
}

}  // namespace sys
