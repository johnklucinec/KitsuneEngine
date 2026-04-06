entt::registry registry;

// Renderer contexts
registry.ctx().emplace<VkContext>();
registry.ctx().emplace<SwapchainState>();
registry.ctx().emplace<FrameState>();
registry.ctx().emplace<PipelineState>();
registry.ctx().emplace<SceneResources>();

// App config
registry.ctx().emplace<Settings>();

// Player entity — replaces camera/walker globals
auto player = registry.create();
registry.emplace<Transform>(player, glm::vec3{ 0.f, 0.f, -6.f }, glm::quat{}, glm::vec3{ 1.f });
registry.emplace<Camera>(player, Camera{ .fov = 90.f });
registry.emplace<PlayerController>(player, PlayerController{ .sensitivity = 2.70f });
registry.emplace<Velocity>(player);
registry.emplace<Input>(player);


// Frame Pacer Hints:

// // ── Setup ──────────────────────────────────────────────
// registry.ctx().emplace<Settings>();
// sys::frame_pacer_init(registry);   // acquires HANDLE, sets scheduler period

// // ── Main loop ──────────────────────────────────────────
// while (running) {
//     sys::frame_pacer(registry);    // first — sleeps, then writes deltaTime/fps

//     auto& fp = registry.ctx().get<FramePacerState>();
//     // fp.deltaTime   → seconds since last frame
//     // fp.currentFps  → measured FPS
//     // fp.deltaUs     → raw microseconds

//     sys::input(registry);
//     sys::render(registry);
//     // ...
// }

// // ── Shutdown ───────────────────────────────────────────
// sys::frame_pacer_shutdown(registry);  // CloseHandle + timeEndPeriod
