#pragma once
#include <SDL3/SDL_video.h>
#include <cstdint>

struct WindowContext
{
  static constexpr uint32_t kDefaultWidth  = 1600;
  static constexpr uint32_t kDefaultHeight = 900;

  uint32_t    width  = kDefaultWidth;
  uint32_t    height = kDefaultHeight;
  SDL_Window* window = nullptr;

  WindowContext(const char* title, SDL_WindowFlags flags, uint32_t w = kDefaultWidth, uint32_t h = kDefaultHeight)
      : width(w)
      , height(h)
      , window(SDL_CreateWindow(title, static_cast<int>(w), static_cast<int>(h), flags))
  {
  }

  operator SDL_Window*() const { return window; }
  float aspectRatio() const { return static_cast<float>(width) / static_cast<float>(height); }
};
