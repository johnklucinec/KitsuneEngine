#include "core/settings.hpp"
#include "utils/frame_pacer_utils.hpp"
#include "components/frame_pacer.hpp"
#include "systems/frame_pacer.hpp"

#pragma comment(lib, "winmm.lib")
#include <mmsystem.h>

namespace sys {

void frame_pacer_init(entt::registry& reg)
{
  auto& fs  = reg.ctx().emplace<FramePacerState>();
  auto& cfg = reg.ctx().get<Settings>();
  reg.ctx().emplace<FrameTime>();

  // Acquire a high-resolution waitable timer (Win10 1803+).
  fs.timerHandle = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);

  // Lower the OS scheduler granularity to its hardware minimum.
  TIMECAPS caps{};
  timeGetDevCaps(&caps, sizeof caps);
  timeBeginPeriod(caps.wPeriodMin);
  fs.schedulerPeriodMs = caps.wPeriodMin;

  // Elevate this thread so OS preemption doesn't introduce drift.
  SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

  // Seed timing from current settings.
  fs.frameDuration     = utils::pacer_to_duration(cfg.fps_max);
  const auto now       = FramePacerState::Clock::now();
  fs.nextFrameDeadline = now + fs.frameDuration;
  fs.frameStart        = now;
}

void frame_pacer(entt::registry& reg)
{
  auto& fs  = reg.ctx().get<FramePacerState>();
  auto& ft  = reg.ctx().get<FrameTime>();
  auto& cfg = reg.ctx().get<Settings>();

  // Sync frame duration if fps_max changed at runtime.
  const auto wantedDuration = utils::pacer_to_duration(cfg.fps_max);
  if(wantedDuration != fs.frameDuration)
    fs.frameDuration = wantedDuration;

  // Sleep until the next deadline (no-op when frameDuration == zero).
  if(fs.frameDuration != FramePacerState::Duration::zero())
    utils::pacer_sleep_until(fs.nextFrameDeadline, fs);

  // Capture actual wake-up time and update delta metrics.
  const auto now = FramePacerState::Clock::now();
  ft.deltaUs     = std::chrono::duration_cast<std::chrono::microseconds>(now - fs.frameStart).count();
  ft.deltaTime   = static_cast<float>(ft.deltaUs) * 1e-6f;
  ft.fps         = ft.deltaUs > 0 ? 1e6f / static_cast<float>(ft.deltaUs) : 0.0f;
  fs.frameStart  = now;

  // Advance deadline; reset if we're more than 4 frames behind
  // (e.g. after a debugger pause or system stall) to avoid a catch-up storm.
  if(fs.frameDuration != FramePacerState::Duration::zero())
  {
    fs.nextFrameDeadline += fs.frameDuration;

    // Spiral-of-death guard: if we fall >4 frames behind (debugger pause, OS stall), reset rather than catching up with a burst of frames.
    if(fs.nextFrameDeadline < now - fs.frameDuration * 4)
      fs.nextFrameDeadline = now + fs.frameDuration;
  }
}

void frame_pacer_shutdown(entt::registry& reg)
{
  auto& fs = reg.ctx().get<FramePacerState>();

  if(fs.timerHandle)
  {
    CloseHandle(fs.timerHandle);
    fs.timerHandle = nullptr;
  }

  // Restore default scheduler period.
  TIMECAPS caps{};
  timeGetDevCaps(&caps, sizeof caps);
  timeEndPeriod(caps.wPeriodMin);
}

}  // namespace sys
