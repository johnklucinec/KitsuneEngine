#define VOLK_IMPLEMENTATION
#define VMA_IMPLEMENTATION
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include "entt/entity/registry.hpp"
#include "core/settings.hpp"
#include "renderer.hpp"

#include "renderer/context.hpp"
#include "renderer/swapchain.hpp"
#include "renderer/frame.hpp"
#include "renderer/resources.hpp"
#include "renderer/pipeline.hpp"
#include "renderer/renderer.hpp"
#include "window.hpp"

#include <SDL3/SDL.h>
namespace Renderer {
void init(entt::registry& registry)
{
  auto& settings = registry.ctx().get<Settings>();

  // Renderer contexts
  auto& ctx = registry.ctx().emplace<VkContext>();
  auto& sc  = registry.ctx().emplace<SwapchainState>();
  auto& fs  = registry.ctx().emplace<FrameState>();
  auto& res = registry.ctx().emplace<SceneResources>();
  auto& ps  = registry.ctx().emplace<PipelineState>();

  chk(SDL_Init(SDL_INIT_VIDEO));
  SDL_SetHint(SDL_HINT_MOUSE_RELATIVE_SYSTEM_SCALE, "0");  // ignore OS accel

  auto& window = registry.ctx().emplace<WindowContext>("KitsuneEngine", SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
  assert(window.window != nullptr);
  SDL_SetWindowRelativeMouseMode(window, true);  // enable raw input

  chk(SDL_Vulkan_LoadLibrary(NULL));
  volkInitialize();

  // TODO: Move to settings
  ctx.deviceIndex   = settings.device;
  fs.framesInFlight = settings.reduce_buffering ? 1 : MAX_FRAMES_IN_FLIGHT;

  initContext(registry);
  initSwapchain(registry);
  initFrameState(registry);
  initSceneResources(registry);
  initPipeline(registry);
}

void deviceWaitIdle(entt::registry& registry)
{
  auto& ctx = registry.ctx().emplace<VkContext>();
  chk(vkDeviceWaitIdle(ctx.device));  // Wait until the GPU has completed all outstanding operations
}

void shutdown(entt::registry& registry)
{
  auto& ctx    = registry.ctx().get<VkContext>();
  auto& sc     = registry.ctx().get<SwapchainState>();
  auto& fs     = registry.ctx().get<FrameState>();
  auto& res    = registry.ctx().get<SceneResources>();
  auto& ps     = registry.ctx().get<PipelineState>();
  auto& window = registry.ctx().get<WindowContext>();

  destroyPipeline(registry);
  destroySceneResources(registry);
  destroyFrameState(registry);
  destroySwapchain(registry);
  destroyContext(registry);

  SDL_DestroyWindow(window);
  SDL_QuitSubSystem(SDL_INIT_VIDEO);
  SDL_Quit();
}
}  // namespace Renderer
