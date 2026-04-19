#include <entt/entt.hpp>
#include "systems/settings.hpp"
#include <charconv>

#include "renderer/swapchain.hpp"
#include "components/input.hpp"
#include "core/settings.hpp"
#include "core/app.hpp"

// Parse function prototype
Settings parseArgs(int argc, char* argv[]);


// Emplaces settings into the registry
void sys::settings_init(entt::registry& registry, int argc, char* argv[])
{
  auto& settings = registry.ctx().get<Settings>();
  settings       = parseArgs(argc, argv);
}


// One-shot actions
void sys::settings_loop(entt::registry& registry)
{
  const auto& in       = registry.ctx().get<Input>();
  auto&       app      = registry.ctx().get<AppState>();
  auto&       settings = registry.ctx().get<Settings>();
  auto&       sc       = registry.ctx().get<SwapchainState>();

  if(key_just_pressed(in, Key::Escape))
    app.running = false;

  if(key_down(in, Key::LAlt) && key_just_pressed(in, Key::Return))
  {
    settings.fullscreen  = !settings.fullscreen;
    sc.needsFullscreen   = true;
    app.resize_swapchain = true;
  }

  if(key_down(in, Key::LAlt) && key_just_pressed(in, Key::Z))
    settings.show_ui = !settings.show_ui;
}


// ========================================
// Parsing helper function
// Should I move this to a util file?
Settings parseArgs(int argc, char* argv[])
{
  Settings s{};

  auto parseFloat = [&](std::string_view name, float lo, float hi, float& dst, int& i) {
    if(i + 1 < argc)
    {
      float            value{};
      std::string_view next{ argv[i + 1] };
      auto [ptr, ec] = std::from_chars(next.data(), next.data() + next.size(), value);
      if(ec == std::errc{} && value >= lo && value <= hi)
      {
        dst = value;
        ++i;
        return;
      }
    }
    std::fprintf(stderr, "Invalid %.*s\n", (int)name.size(), name.data());
  };

  auto parseUint = [&](std::string_view name, uint32_t& dst, int& i) {
    if(i + 1 < argc)
    {
      uint32_t         value{};
      std::string_view next{ argv[i + 1] };
      auto [ptr, ec] = std::from_chars(next.data(), next.data() + next.size(), value);
      if(ec == std::errc{})
      {
        dst = value;
        ++i;
        return;
      }
    }
    std::fprintf(stderr, "Invalid %.*s\n", (int)name.size(), name.data());
  };

  for(int i = 1; i < argc; ++i)
  {
    std::string_view arg{ argv[i] };

    if(arg == "-device")
      parseUint("device", s.device, i);
    else if(arg == "-fov")
      parseFloat("fov", 30.f, 160.f, s.fov, i);
    else if(arg == "-fps_max")
      parseFloat("fps_max", 0.f, 9999.f, s.fps_max, i);
    else if(arg == "-fullscreen")
      s.fullscreen = true;
    else if(arg == "-reduce_buffering")
      s.reduce_buffering = true;
    else if(arg == "-sensitivity")
      parseFloat("sensitivity", 0.f, 100.f, s.sensitivity, i);
    else if(arg == "-vsync")
      s.vsync = true;
  }

  return s;
}
