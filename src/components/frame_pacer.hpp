#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <chrono>
#include <cstdint>


struct FrameTime
{
  float   deltaTime{ 1.0f / 60.f };
  float   fps{ 60.0f };
  int64_t deltaUs{ 16'667LL };
};

struct FramePacerState
{
  using Clock     = std::chrono::high_resolution_clock;
  using Duration  = Clock::duration;
  using TimePoint = Clock::time_point;

  Duration  frameDuration{};  // zero = unlimited
  TimePoint nextFrameDeadline{};
  TimePoint frameStart{};
  HANDLE    timerHandle{ nullptr };
  int       schedulerPeriodMs{ 1 };
};
