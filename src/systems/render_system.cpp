/* Copyright (c) 2025-2026, Johnny Klucinec */
#include <entt/entity/registry.hpp>
#include "render_system.hpp"
#include "core/app.hpp"

#include "app_controls_system.hpp"
#include "input_system.hpp"
#include "camera_system.hpp"
#include "frame_pacer_system.hpp"
#include "player_movement_system.hpp"

#include "renderer/renderer.hpp"
#include "renderer/render_loop.hpp"

void System::render(entt::registry& registry)
{
  auto& app = registry.ctx().get<AppState>();

  Renderer::init(registry);
  RenderLoop::init(registry);
  System::framePacerInit(registry);

  // ========================================
  // Render Loop
  while(app.running)
  {
    // ==== Wait on Fence (sync) ====
    RenderLoop::waitForFences(registry);

    // Setup frame pacing; sleeps remaining budget
    System::framePacer(registry);

    // ==== Aquire Next Image ====
    RenderLoop::acquireNextImage(registry);

    // === Polling Events ===
    System::input(registry);
    System::camera(registry);
    System::playerMovement(registry);

    // ==== Update Shader Data ====
    RenderLoop::updateShaderData(registry);
    RenderLoop::updateInstanceBuffer(registry);

    // ==== Record Command Buffer (also builds HUD) ====
    RenderLoop::recordCommandBuffer(registry);

    // ==== Submit Command Buffer (submit to graphics queue) ===
    RenderLoop::submitCommandBuffer(registry);
    RenderLoop::advanceFrameIndex(registry);

    // === Present Image ===
    RenderLoop::presentImage(registry);

    // === Check Settings and Swapchain ===
    System::appControls(registry);
    RenderLoop::rebuildSwapchain(registry);
  }

  // ========================================
  // Cleaning Up
  Renderer::deviceWaitIdle(registry);
  System::framePacerShutdown(registry);
  RenderLoop::shutdown(registry);
  Renderer::shutdown(registry);
}
