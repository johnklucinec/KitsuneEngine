/* Copyright (c) 2025-2026, Johnny Klucinec
 * SPDX-License-Identifier: MIT */

#define VOLK_IMPLEMENTATION
#define VMA_IMPLEMENTATION
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include "systems/hud.hpp"

#include <entt/entt.hpp>
#include "core/settings.hpp"
#include "core/app.hpp"
#include <SDL3/SDL.h>
#include "components/transform.hpp"
#include "systems/input.hpp"
#include "systems/frame_pacer.hpp"
#include "components/input.hpp"
#include "systems/camera.hpp"
#include "components/camera.hpp"
#include "systems/player_movement.hpp"

#include "renderer/context.hpp"
#include "renderer/swapchain.hpp"
#include "renderer/frame.hpp"
#include "renderer/resources.hpp"
#include "renderer/pipeline.hpp"
#include "components/window.hpp"

#include "components/tags.hpp"
#include "components/frame_pacer.hpp"

int run(entt::registry& registry)
{
  auto& app      = registry.ctx().get<AppState>();
  auto& settings = registry.ctx().get<Settings>();

  // Renderer contexts
  auto& ctx = registry.ctx().emplace<VkContext>();
  auto& sc  = registry.ctx().emplace<SwapchainState>();
  auto& fs  = registry.ctx().emplace<FrameState>();
  auto& res = registry.ctx().emplace<SceneResources>();
  auto& ps  = registry.ctx().emplace<PipelineState>();

  chk(SDL_Init(SDL_INIT_VIDEO));
  SDL_SetHint(SDL_HINT_MOUSE_RELATIVE_SYSTEM_SCALE, "0");  // ignore OS accel

  auto& window = registry.ctx().emplace<WindowContext>("Kitsune", SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
  assert(window.window != nullptr);
  SDL_SetWindowRelativeMouseMode(window, true);  // enable raw input

  chk(SDL_Vulkan_LoadLibrary(NULL));
  volkInitialize();

  ctx.deviceIndex   = settings.device;
  fs.framesInFlight = settings.reduce_buffering ? 1 : MAX_FRAMES_IN_FLIGHT;

  renderer::initContext(ctx, window);
  renderer::initSwapchain(sc, ctx, window, settings.fullscreen);
  renderer::initFrameState(fs, ctx, sc);
  renderer::initSceneResources(res, ctx, fs);
  renderer::initPipeline(ps, ctx, sc, res);

  // Mark player for camera initialization
  auto playerView = registry.view<PlayerTag>();
  for(auto player : playerView)
  {
    registry.emplace_or_replace<DirtyCameraProjection>(player);  // Calculate ProjectionMatrix for first frame
    registry.emplace_or_replace<DirtyCameraTransform>(player);   // Calculate WorldMatrix for first frame
  }

  for(auto i = 0; i < INSTANCE_COUNT; i++)
  {
    auto instancePos        = glm::vec3((float)(i - 1) * 3.0f, 0.0f, 0.0f);
    res.shaderData.model[i] = glm::translate(glm::mat4(1.0f), instancePos);  // static, no rotation
  }

  for(uint32_t i = 0; i < fs.framesInFlight; i++)
    memcpy(res.shaderDataBuffers[i].allocationInfo.pMappedData, &res.shaderData, sizeof(ShaderData));

  entt::entity playerEntity = registry.view<PlayerTag>().front();
  assert(playerEntity != entt::null);

  // ========================================
  // Render Loop
  sys::frame_pacer_init(registry);
  sys::hud_init(registry);

  while(app.running)
  {
    // ==== Wait on Fence (sync) ====
    chk(vkWaitForFences(ctx.device, 1, &fs.frames[fs.frameIndex].fence, true, UINT64_MAX));  // Wait for last frame GPU is working on
    chk(vkResetFences(ctx.device, 1, &fs.frames[fs.frameIndex].fence));                      // Reset for next submission

    sys::hud_begin(registry);

    // Setup frame pacing; sleeps remaining budget
    sys::frame_pacer(registry);
    const auto& fp = registry.ctx().get<FramePacerState>();

    // ==== Aquire Next Image ====
    if(chkSwapchain(vkAcquireNextImageKHR(ctx.device, sc.swapchain, UINT64_MAX, fs.frames[fs.frameIndex].presentSem, VK_NULL_HANDLE, &sc.imageIndex)))
      app.resize_swapchain = true;

    // === Polling Events ===
    sys::input(registry);
    sys::camera(registry, sc.extent.width, sc.extent.height);
    sys::player_movement(registry, fp.deltaTime);

    // ==== Update Shader Data ====
    const auto& world         = registry.get<WorldMatrix>(playerEntity);
    const auto& proj          = registry.get<ProjectionMatrix>(playerEntity);
    res.shaderData.view       = world.matrix;
    res.shaderData.projection = proj.matrix;
    memcpy(res.shaderDataBuffers[fs.frameIndex].allocationInfo.pMappedData, &res.shaderData, sizeof(ShaderData));

    // ==== Record Command Buffer (build command buffer) ====
    auto cb = fs.frames[fs.frameIndex].commandBuffer;
    chk(vkResetCommandBuffer(cb, 0));  // Reset command buffer
    VkCommandBufferBeginInfo cbBI{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    chk(vkBeginCommandBuffer(cb, &cbBI));  // Start recording to cb

    // Issue layout transitions for swapchain image and depth image
    // clang-format off
    std::array<VkImageMemoryBarrier2, 2> outputBarriers{
      VkImageMemoryBarrier2{
          .sType         = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
          .srcStageMask  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,  // pipeline stage(s) to wait on
          .srcAccessMask = 0,                                                // memory writes to make available to the GPU
          .dstStageMask  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,  // where and what those writes must be visible to
          .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
          .oldLayout     = VK_IMAGE_LAYOUT_UNDEFINED,           // dont need previous contents
          .newLayout     = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,  // use as color attachment for rendering
          .image         = sc.images[sc.imageIndex],
          .subresourceRange{ .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 },
      },

      VkImageMemoryBarrier2{
          .sType         = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
          .srcStageMask  = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,   // pipeline stage(s) to wait on
          .srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,  // memory writes to make available to the GPU
          .dstStageMask  = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,  // where and what those writes must be visible to
          .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
          .oldLayout     = VK_IMAGE_LAYOUT_UNDEFINED,           // dont need previous contents
          .newLayout     = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,  // use as color attachment for rendering
          .image         = sc.depthImage,
          .subresourceRange{ .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, .levelCount = 1, .layerCount = 1 },
      }
    };  // clang-format on
    VkDependencyInfo barrierDependencyInfo{
      .sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
      .imageMemoryBarrierCount = 2,
      .pImageMemoryBarriers    = outputBarriers.data(),
    };
    vkCmdPipelineBarrier2(cb, &barrierDependencyInfo);  // Insert those two barriers into the current command buffer


    // Define how attachments are used: Dynamic Rendering
    VkRenderingAttachmentInfo colorAttachmentInfo{
      .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView   = sc.imageViews[sc.imageIndex],
      .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
      .loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp     = VK_ATTACHMENT_STORE_OP_STORE,
      .clearValue{ .color{ 0.0f, 0.0f, 0.0f, 1.0f } },
    };
    VkRenderingAttachmentInfo depthAttachmentInfo{
      .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView   = sc.depthImageView,
      .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
      .loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp     = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .clearValue  = { .depthStencil = { 1.0f, 0 } },
    };

    // Start dynamic render pass
    VkRenderingInfo renderingInfo{
      .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
      .renderArea{ .extent{ .width = sc.extent.width, .height = sc.extent.height } },
      .layerCount           = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments    = &colorAttachmentInfo,
      .pDepthAttachment     = &depthAttachmentInfo,
    };
    vkCmdBeginRendering(cb, &renderingInfo);

    // Start recording GPU commands [vkCmd*]
    VkViewport vp{
      .width    = static_cast<float>(sc.extent.width),
      .height   = static_cast<float>(sc.extent.height),
      .minDepth = 0.0f,
      .maxDepth = 1.0f,
    };
    vkCmdSetViewport(cb, 0, 1, &vp);
    VkRect2D scissor{
      .extent{ .width = sc.extent.width, .height = sc.extent.height }
    };

    // Bind resources involved in rendering the 3D objects
    vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, ps.pipeline);
    vkCmdSetScissor(cb, 0, 1, &scissor);
    vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, ps.layout, 0, 1, &ps.descSet, 0, nullptr);
    VkDeviceSize vOffset{ 0 };
    vkCmdBindVertexBuffers(cb, 0, 1, &res.vBuffer, &vOffset);
    vkCmdBindIndexBuffer(cb, res.vBuffer, res.indexOffset, VK_INDEX_TYPE_UINT16);

    // Pass the address of the current frame's shader data buffer via a push constant to the shaders
    vkCmdPushConstants(cb, ps.layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(VkDeviceAddress), &res.shaderDataBuffers[fs.frameIndex].deviceAddress);

    // Draw mesh
    vkCmdDrawIndexed(cb, res.indexCount, INSTANCE_COUNT, 0, 0, 0);  // commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstanceID

    // Build and draw HUD
    sys::hud_draw(registry);

    vkCmdEndRendering(cb);  // Finish current render pass

    // Transition swapchain image to a layout required for presentation
    VkImageMemoryBarrier2 barrierPresent{
      .sType         = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
      .srcStageMask  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
      .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      .dstStageMask  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
      .dstAccessMask = 0,
      .oldLayout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .newLayout     = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
      .image         = sc.images[sc.imageIndex],
      .subresourceRange{ .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 },
    };
    VkDependencyInfo barrierPresentDependencyInfo{
      .sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
      .imageMemoryBarrierCount = 1,
      .pImageMemoryBarriers    = &barrierPresent,
    };
    vkCmdPipelineBarrier2(cb, &barrierPresentDependencyInfo);
    chk(vkEndCommandBuffer(cb));  // End recording to the command buffer

    // ==== Submit Command Buffer (submit to graphics queue) ===
    // Only one graphics queue (no compute or ray tracing)
    VkSemaphoreSubmitInfo waitSemaphoreInfo{
      .sType     = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
      .semaphore = fs.frames[fs.frameIndex].presentSem,
      .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
    };
    VkCommandBufferSubmitInfo commandBufferInfo{
      .sType         = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
      .commandBuffer = cb,
    };
    VkSemaphoreSubmitInfo signalSemaphoreInfo{
      .sType     = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
      .semaphore = fs.renderSemaphores[sc.imageIndex],
      .stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
    };
    VkSubmitInfo2 submitInfo{
      .sType                    = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
      .waitSemaphoreInfoCount   = 1,
      .pWaitSemaphoreInfos      = &waitSemaphoreInfo,
      .commandBufferInfoCount   = 1,
      .pCommandBufferInfos      = &commandBufferInfo,
      .signalSemaphoreInfoCount = 1,
      .pSignalSemaphoreInfos    = &signalSemaphoreInfo,
    };
    chk(vkQueueSubmit2(ctx.queue, 1, &submitInfo, fs.frames[fs.frameIndex].fence));

    fs.frameIndex = (fs.frameIndex + 1) % fs.framesInFlight;  // calculate frame index for next render loop iteration

    // === Present Image ===
    VkPresentInfoKHR presentInfo{
      .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores    = &fs.renderSemaphores[sc.imageIndex],
      .swapchainCount     = 1,
      .pSwapchains        = &sc.swapchain,
      .pImageIndices      = &sc.imageIndex,
    };
    if(chkSwapchain(vkQueuePresentKHR(ctx.queue, &presentInfo)))
      app.resize_swapchain = true;

    // One-shot actions TODO: Move to own system
    const auto& in = registry.get<Input>(playerEntity);
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


    if(sc.needsFullscreen)
    {
      if(settings.fullscreen)
      {
        SDL_DisplayID          displayID = SDL_GetPrimaryDisplay();
        const SDL_DisplayMode* mode      = SDL_GetCurrentDisplayMode(displayID);
        chk(SDL_SetWindowFullscreenMode(window, mode));
        chk(SDL_SetWindowFullscreen(window, true));
        chk(SDL_SyncWindow(window));
      }
      else
      {
        chk(SDL_SetWindowFullscreen(window, false));
      }
      sc.needsFullscreen   = false;
      app.resize_swapchain = true;
    }

    if(app.resize_swapchain)
    {
      app.resize_swapchain = false;

      renderer::rebuildSwapchain(sc, ctx);
      renderer::syncFrameSemaphores(fs, ctx, sc);
      registry.emplace_or_replace<DirtyCameraProjection>(playerEntity);
    }
  }

  // ========================================
  // Cleaning Up
  chk(vkDeviceWaitIdle(ctx.device));  // Wait until the GPU has completed all outstanding operations

  sys::frame_pacer_shutdown(registry);

  sys::hud_shutdown(registry);

  renderer::destroyPipeline(ps, ctx);
  renderer::destroySceneResources(res, ctx);
  renderer::destroyFrameState(fs, ctx);
  renderer::destroySwapchain(sc, ctx);
  renderer::destroyContext(ctx);

  SDL_DestroyWindow(window);
  SDL_QuitSubSystem(SDL_INIT_VIDEO);
  SDL_Quit();

  return 0;
}


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
    {
      s.reduce_buffering = true;
    }
    else if(arg == "-sensitivity")
    {
      parseFloat("sensitivity", 0.f, 100.f, s.sensitivity, i);
      // camera.sensitivity = s.sensitivity;
      // TODO: Refactor into settings?
    }
    else if(arg == "-vsync")
      s.vsync = true;
  }

  return s;
}
