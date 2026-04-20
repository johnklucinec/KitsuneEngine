#include <entt/entity/registry.hpp>
#include "app_controls_system.hpp"

#include "renderer/swapchain.hpp"
#include "input.hpp"
#include "core/settings.hpp"
#include "core/app.hpp"

void System::appControls(entt::registry& registry)
{
  const auto& in       = registry.ctx().get<Input>();
  auto&       app      = registry.ctx().get<AppState>();
  auto&       settings = registry.ctx().get<Settings>();
  auto&       sc       = registry.ctx().get<SwapchainState>();

  if(keyJustPressed(in, Key::Escape))
    app.running = false;

  if(keyDown(in, Key::LAlt) && keyJustPressed(in, Key::Return))
  {
    settings.fullscreen  = !settings.fullscreen;
    sc.needsFullscreen   = true;
    app.resize_swapchain = true;
  }

  if(keyDown(in, Key::LAlt) && keyJustPressed(in, Key::Z))
    settings.show_ui = !settings.show_ui;
}
