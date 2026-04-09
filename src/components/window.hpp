#pragma once
#include <SDL3/SDL_video.h>
#include <cstdint>

struct WindowContext
{
  SDL_Window* window = nullptr;
  uint32_t    width  = 1600;
  uint32_t    height = 900;

  operator SDL_Window*() const { return window; }

  float aspectRatio() const { return static_cast<float>(width) / static_cast<float>(height); }
};
