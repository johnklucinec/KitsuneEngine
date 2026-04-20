#pragma once

#include <stdint.h>
#include <glm/glm.hpp>

struct Settings
{
  uint32_t   device           = 0;       // GPU deviceIndex
  float      fov              = 103.0f;  // HFOV
  float      fps_max          = 120.0f;
  bool       fullscreen       = false;
  bool       reduce_buffering = false;
  float      sensitivity      = 2.70f;
  bool       show_ui          = true;
  bool       vsync            = false;
};
