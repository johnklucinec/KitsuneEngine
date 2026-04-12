#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <chrono>
#include <cstdint>

struct FramePacerState
{
  using Clock     = std::chrono::high_resolution_clock;
  using Duration  = Clock::duration;
  using TimePoint = Clock::time_point;

  Duration  frameDuration{};  // zero = unlimited
  TimePoint deadline{};
  TimePoint lastWake{};
  int64_t   deltaUs{ 16'667LL };  // seeded to ~60 fps
  float     deltaTime{ 1.0f / 60.f };
  float     currentFps{ 60.0f };
  HANDLE    timerHandle{ nullptr };
  int       schedulerPeriodMs{ 1 };
};
