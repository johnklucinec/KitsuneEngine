#include <entt/entity/fwd.hpp>
#include <entt/entity/registry.hpp>

#include "core/settings.hpp"
#include "core/app.hpp"

#include "render_loop.hpp"
#include "mesh.hpp"
#include "renderer/context.hpp"
#include "renderer/swapchain.hpp"
#include "renderer/frame.hpp"
#include "renderer/resources.hpp"
#include "renderer/pipeline.hpp"

#include "window.hpp"
#include "transform.hpp"
#include "camera.hpp"
#include "tags.hpp"
#include "types.hpp"

#include "hud_system.hpp"

struct MeshDrawParams
{
  uint32_t instanceCount;
  uint32_t firstInstance;
};

// TODO: Move this somewhere else or use the one in camera.
inline glm::mat4 toMatrix(const Transform& t)
{
  return glm::translate(glm::mat4{ 1.f }, t.position) * glm::mat4_cast(t.rotation) * glm::scale(glm::mat4{ 1.f }, t.scale);
}

namespace {
struct RenderCache
{
  entt::entity                               playerEntity;
  std::vector<InstanceData>                  staticInstances;
  std::vector<std::pair<uint32_t, uint32_t>> staticMesh;
};
static RenderCache rc{};
}  // namespace

void RenderLoop::init(entt::registry& registry)
{
  auto& fs  = registry.ctx().get<FrameState>();
  auto& res = registry.ctx().get<SceneResources>();

  // TODO: Textures and meshes should use MeshInstance and TextureInstance??
  int32_t suzanneMesh = Renderer::loadMesh("assets/models/suzanne.gltf", registry);

  // ========================================
  // Load Textures
  // Eventually, my texture loading system should do this I think.
  uint32_t tex0 = Renderer::loadTexture("assets/models/suzanne0.ktx", registry);
  uint32_t tex1 = Renderer::loadTexture("assets/models/suzanne1.ktx", registry);
  uint32_t tex2 = Renderer::loadTexture("assets/models/suzanne2.ktx", registry);

  // Call after loading textures
  Renderer::updateDescriptorSets(registry);

  for(int i = 0; i < 3; ++i)
  {
    auto e = registry.create();
    registry.emplace<Transform>(e, glm::vec3(i * 3.f - 3.f, 0.f, 0.f), glm::identity<glm::quat>(), glm::vec3{ 1.f });
    registry.emplace<MeshInstance>(e, 0u);
    registry.emplace<TextureInstance>(e, tex0);
  }

  for(int i = 0; i < 2; ++i)
  {
    auto e = registry.create();
    registry.emplace<Transform>(e, glm::vec3(i * 3.f - 1.5f, 3.f, 0.f), glm::identity<glm::quat>(), glm::vec3{ 1.f });
    registry.emplace<MeshInstance>(e, 0u);
    registry.emplace<TextureInstance>(e, tex1);
  }

  // Separate static entities into rc.staticInstances once
  {
    uint32_t slot = 0;
    for(uint32_t meshIdx = 0; meshIdx < res.meshes.size(); ++meshIdx)
    {
      // everything NOT dirty = static
      auto     view  = registry.view<Transform, MeshInstance, TextureInstance>(entt::exclude<DirtyTransform>);
      uint32_t count = 0;

      for(auto e : view)
      {
        if(view.get<MeshInstance>(e).meshIndex != meshIdx)
          continue;

        const auto& t  = view.get<Transform>(e);
        const auto& ti = view.get<TextureInstance>(e);
        rc.staticInstances.push_back({ toMatrix(t), ti.textureIndex });
        ++count;
      }

      rc.staticMesh.push_back({ slot, count });
      slot += count;
    }
  }

  rc.playerEntity = registry.view<PlayerTag>().front();
  assert(rc.playerEntity != entt::null);

  // Also controls HUD
  System::hudInit(registry);
}


void RenderLoop::waitForFences(entt::registry& registry)
{
  auto& ctx   = registry.ctx().get<VkContext>();
  auto& frame = registry.ctx().get<FrameState>().currentFrame();

  chk(vkWaitForFences(ctx.device, 1, &frame.fence, true, UINT64_MAX));  // Wait for last frame GPU is working on
  chk(vkResetFences(ctx.device, 1, &frame.fence));                      // Reset for next submission
}


void RenderLoop::acquireNextImage(entt::registry& registry)
{
  auto& app   = registry.ctx().get<AppState>();
  auto& ctx   = registry.ctx().get<VkContext>();
  auto& sc    = registry.ctx().get<SwapchainState>();
  auto& frame = registry.ctx().get<FrameState>().currentFrame();

  if(chkSwapchain(vkAcquireNextImageKHR(ctx.device, sc.swapchain, UINT64_MAX, frame.presentSem, VK_NULL_HANDLE, &sc.imageIndex)))
    app.resize_swapchain = true;
}


void RenderLoop::updateShaderData(entt::registry& registry)
{
  auto& fs  = registry.ctx().get<FrameState>();
  auto& res = registry.ctx().get<SceneResources>();

  const auto& world = registry.get<CameraViewMatrix>(rc.playerEntity);
  const auto& proj  = registry.get<ProjectionMatrix>(rc.playerEntity);

  auto* fd       = static_cast<ShaderData*>(res.shaderDataBuffers[fs.frameIndex].allocationInfo.pMappedData);
  fd->view       = world.matrix;
  fd->projection = proj.matrix;
}


void RenderLoop::recordCommandBuffer(entt::registry& registry)
{
  auto& ctx = registry.ctx().get<VkContext>();
  auto& sc  = registry.ctx().get<SwapchainState>();
  auto& fs  = registry.ctx().get<FrameState>();
  auto& ps  = registry.ctx().get<PipelineState>();
  auto& res = registry.ctx().get<SceneResources>();

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
      .srcStageMask  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,	// pipeline stage(s) to wait on
      .srcAccessMask = 0,                                        				// memory writes to make available to the GPU
      .dstStageMask  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,	// where and what those writes must be visible to
      .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      .oldLayout     = VK_IMAGE_LAYOUT_UNDEFINED,       		// dont need previous contents
      .newLayout     = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,	// use as color attachment
      .image         = sc.images[sc.imageIndex],
      .subresourceRange{ .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 }
  },

    VkImageMemoryBarrier2{
    	.sType         = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
      .srcStageMask  = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,		// pipeline stage(s) to wait on
      .srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,	// memory writes to make available to the GPU
      .dstStageMask  = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,	// where and what those writes must be visible to
      .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
      .oldLayout     = VK_IMAGE_LAYOUT_UNDEFINED, 					// dont need previous contents
      .newLayout     = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,	// use as color attachment for rendering
      .image         = sc.depthImage,
      .subresourceRange{ .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, .levelCount = 1, .layerCount = 1 } }
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

  // Bind pipeline, descriptor set, and push constants once for all meshes
  vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, ps.pipeline);
  vkCmdSetScissor(cb, 0, 1, &scissor);
  vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, ps.layout, 0, 1, &ps.descSet, 0, nullptr);

  PushConstants pushConstants{
    .shaderData = res.shaderDataBuffers[fs.frameIndex].deviceAddress,
    .instances  = res.instanceBuffers[fs.frameIndex].deviceAddress,
  };
  // Pass the address of the current frame's shader data buffer via a push constant to the shaders
  vkCmdPushConstants(cb, ps.layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &pushConstants);

  // Per-mesh: bind its buffer and draw its instances
  uint32_t runningOffset = 0;
  for(uint32_t meshIdx = 0; meshIdx < res.meshes.size(); ++meshIdx)
  {
    uint32_t count = 0;
    auto     view  = registry.view<MeshInstance>();

    for(auto e : view)
      if(view.get<MeshInstance>(e).meshIndex == meshIdx)
        ++count;

    if(count == 0)
      continue;

    const auto&  mesh = res.meshes[meshIdx];
    VkDeviceSize vOffset{ 0 };
    vkCmdBindVertexBuffers(cb, 0, 1, &mesh.buffer, &vOffset);
    vkCmdBindIndexBuffer(cb, mesh.buffer, mesh.indexOffset, VK_INDEX_TYPE_UINT16);
    vkCmdDrawIndexed(cb, mesh.indexCount, count, 0, 0, runningOffset);
    runningOffset += count;
  }

  // Build and draw HUD
  System::hudDraw(registry);

  // Finish current render pass
  vkCmdEndRendering(cb);

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
}

void RenderLoop::updateInstanceBuffer(entt::registry& registry)
{
  auto& fs  = registry.ctx().get<FrameState>();
  auto& res = registry.ctx().get<SceneResources>();

  auto* instances = static_cast<InstanceData*>(res.instanceBuffers[fs.frameIndex].allocationInfo.pMappedData);

  // 1. Copy cached static instances
  std::memcpy(instances, rc.staticInstances.data(), rc.staticInstances.size() * sizeof(InstanceData));

  // 2. Recompute only dirty (dynamic) entities
  uint32_t slot = static_cast<uint32_t>(rc.staticInstances.size());
  for(uint32_t meshIdx = 0; meshIdx < res.meshes.size(); ++meshIdx)
  {
    auto view = registry.view<Transform, MeshInstance, TextureInstance, DirtyTransform>();
    for(auto e : view)
    {
      if(view.get<MeshInstance>(e).meshIndex != meshIdx)
        continue;

      const auto& t     = view.get<Transform>(e);
      const auto& ti    = view.get<TextureInstance>(e);
      instances[slot++] = { toMatrix(t), ti.textureIndex };
    }
  }

  // NOTE: clear<> runs after the instance buffer is already written and submitted,
  // so this is safe for single-pass uploads. If you ever defer writes across
  // multiple in-flight frames, delay this clear until all frames-in-flight
  // have consumed the buffer for this frameIndex.
  registry.clear<DirtyTransform>();
}

void RenderLoop::submitCommandBuffer(entt::registry& registry)
{
  auto& ctx = registry.ctx().get<VkContext>();
  auto& sc  = registry.ctx().get<SwapchainState>();
  auto& fs  = registry.ctx().get<FrameState>();

  auto& frame = fs.frames[fs.frameIndex];
  auto  cb    = frame.commandBuffer;

  // Only one graphics queue (no compute or ray tracing)
  VkSemaphoreSubmitInfo waitSemaphoreInfo{
    .sType     = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
    .semaphore = frame.presentSem,
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
  chk(vkQueueSubmit2(ctx.queue, 1, &submitInfo, frame.fence));
}

// calculate frame index for next render loop iteration
void RenderLoop::advanceFrameIndex(entt::registry& registry)
{
  auto& fs      = registry.ctx().get<FrameState>();
  fs.frameIndex = (fs.frameIndex + 1) % fs.framesInFlight;
}

void RenderLoop::presentImage(entt::registry& registry)
{
  auto& app = registry.ctx().get<AppState>();
  auto& ctx = registry.ctx().get<VkContext>();
  auto& sc  = registry.ctx().get<SwapchainState>();
  auto& fs  = registry.ctx().get<FrameState>();

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
}

void RenderLoop::rebuildSwapchain(entt::registry& registry)
{
  auto& app = registry.ctx().get<AppState>();

  if(app.resize_swapchain)
  {
    auto& ctx      = registry.ctx().get<VkContext>();
    auto& sc       = registry.ctx().get<SwapchainState>();
    auto& fs       = registry.ctx().get<FrameState>();
    auto& settings = registry.ctx().get<Settings>();
    auto& window   = registry.ctx().get<WindowContext>();

    if(sc.needsFullscreen)
    {
      if(settings.fullscreen)
      {
        SDL_DisplayID          displayID = SDL_GetPrimaryDisplay();
        const SDL_DisplayMode* mode      = SDL_GetCurrentDisplayMode(displayID);
        chk(SDL_SetWindowFullscreenMode(window.window, mode));
        chk(SDL_SetWindowFullscreen(window.window, true));
        chk(SDL_SyncWindow(window.window));
      }
      else
      {
        chk(SDL_SetWindowFullscreen(window.window, false));
      }
      sc.needsFullscreen = false;
    }

    Renderer::rebuildSwapchain(registry);
    Renderer::syncFrameSemaphores(registry);
    window.height = sc.extent.height;
    window.width  = sc.extent.width;
    registry.emplace_or_replace<DirtyCameraProjection>(rc.playerEntity);

    app.resize_swapchain = false;
  }
}

void RenderLoop::shutdown(entt::registry& registry)
{
  System::hudShutdown(registry);
  rc.playerEntity = {};
}
