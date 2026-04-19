/* Copyright (c) 2025-2026, Johnny Klucinec */

#include "core/app.hpp"

#include "systems/settings.hpp"
#include "systems/input.hpp"
#include "systems/camera.hpp"
#include "systems/frame_pacer.hpp"
#include "systems/player_movement.hpp"

#include "renderer/renderer.hpp"
#include "renderer/render_loop.hpp"

// TODO: Put parse args in util file.
// TODO: Rename settings system?
// TODO: use settings util in main, and remove settings_init
// TODO: add update settings (or at least a comment for it)
// TODO: refactor this into render system
// TODO: ensure consistent system comments
// TODO: update ECS README and README.md
int run(entt::registry& registry)
{
  auto& app      = registry.ctx().get<AppState>();

  renderer::init(registry);
  renderloop::init(registry);
  sys::frame_pacer_init(registry);

  // ========================================
  // Render Loop
  while(app.running)
  {
    // ==== Wait on Fence (sync) ====
    renderloop::waitForFences(registry);

    // Setup frame pacing; sleeps remaining budget
    sys::frame_pacer(registry);

    // ==== Aquire Next Image ====
    renderloop::acquireNextImage(registry);

    // === Polling Events ===
    sys::input(registry);
    sys::camera(registry);
    sys::player_movement(registry);

    // ==== Update Shader Data ====
    renderloop::updateShaderData(registry);

    // ==== Record Command Buffer (also builds HUD) ====
    renderloop::recordCommandBuffer(registry);

    // ==== Submit Command Buffer (submit to graphics queue) ===
    renderloop::submitCommandBuffer(registry);
    renderloop::advanceFrameIndex(registry);

    // === Present Image ===
    renderloop::presentImage(registry);

    // === Check Settings and Swapchain ===
    sys::settings_loop(registry);
    renderloop::rebuildSwapchain(registry);
  }

  // ========================================
  // Cleaning Up
  renderer::deviceWaitIdle(registry);
  sys::frame_pacer_shutdown(registry);
  renderloop::shutdown(registry);
  renderer::shutdown(registry);

  return 0;
}
