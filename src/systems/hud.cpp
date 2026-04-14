#include <entt/entt.hpp>

#include "systems/hud.hpp"
#include "components/hud.hpp"

#include <volk/volk.h>
#include <imgui.h>
#include <imgui_impl_vulkan.h>
#include <imgui_impl_sdl3.h>
#include <SDL3/SDL.h>

#include "renderer/context.hpp"
#include "renderer/swapchain.hpp"
#include "renderer/pipeline.hpp"
#include "renderer/frame.hpp"

#include "components/window.hpp"
#include "core/settings.hpp"

namespace sys {

// Cached HUD entity stored in context to avoid a view scan every frame
// TODO: Move to singleton?
struct HudEntity
{
  entt::entity value;
};

void hud_init(entt::registry& reg)
{
  auto& window = reg.ctx().get<WindowContext>();
  auto& ctx    = reg.ctx().get<VkContext>();
  auto& sc     = reg.ctx().get<SwapchainState>();
  auto& ps     = reg.ctx().get<PipelineState>();

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  ImGuiIO& io    = ImGui::GetIO();
  io.IniFilename = nullptr;
  io.LogFilename = nullptr;

  ImGuiStyle& style               = ImGui::GetStyle();
  style.WindowBorderSize          = 1.0f;
  style.WindowPadding             = { 8.0f, 6.0f };
  style.ItemSpacing               = { 6.0f, 3.0f };
  style.Colors[ImGuiCol_WindowBg] = { 0.04f, 0.04f, 0.04f, 0.58f };
  style.Colors[ImGuiCol_Border]   = { 0.50f, 0.50f, 0.50f, 0.40f };

  ImGui_ImplSDL3_InitForVulkan(window.window);

  ImGui_ImplVulkan_InitInfo vi{};
  vi.Instance            = ctx.instance;
  vi.PhysicalDevice      = ctx.physicalDevice;
  vi.Device              = ctx.device;
  vi.QueueFamily         = ctx.queueFamily;
  vi.Queue               = ctx.queue;
  vi.MinImageCount       = sc.swapchainCI.minImageCount;
  vi.ImageCount          = static_cast<uint32_t>(sc.images.size());
  vi.ApiVersion          = VK_API_VERSION_1_4;
  vi.DescriptorPool      = ps.descPool;
  vi.UseDynamicRendering = true;

  vi.PipelineInfoMain.MSAASamples                 = VK_SAMPLE_COUNT_1_BIT;
  vi.PipelineInfoMain.PipelineRenderingCreateInfo = {
    .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
    .colorAttachmentCount    = 1,
    .pColorAttachmentFormats = &sc.colorFormat,
    .depthAttachmentFormat   = VK_FORMAT_UNDEFINED,
    .stencilAttachmentFormat = VK_FORMAT_UNDEFINED,
  };

  ImGui_ImplVulkan_Init(&vi);

  // Spawn and cache the single HUD entity
  const auto e = reg.create();
  reg.emplace<Hud>(e);
  reg.ctx().emplace<HudEntity>(e);
}

inline void shutdown(VkDevice device)
{
  vkDeviceWaitIdle(device);
  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplSDL3_Shutdown();
  ImGui::DestroyContext();
}

void hud_draw(entt::registry& reg)
{
  auto& settings = reg.ctx().get<Settings>();
  if(!settings.show_ui)
    return;

  // Begin ImGui frame here
  // TODO: Verify: called right after frame_pacer so DeltaTime includes the sleep and io.Framerate stays accurate
  ImGui_ImplSDL3_NewFrame();
  ImGui_ImplVulkan_NewFrame();
  ImGui::NewFrame();

  auto& frame = reg.ctx().get<FrameState>();

  bool fps_limited = settings.fps_max != 0 || settings.vsync;
  bool fullscreen  = settings.fullscreen;

  constexpr ImGuiWindowFlags kHUDFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar
                                         | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize;

  constexpr ImVec4             kGreen  = { 0.20f, 0.85f, 0.20f, 1.0f };
  constexpr ImVec4             kRed    = { 0.85f, 0.20f, 0.20f, 1.0f };
  static constexpr const char* kDots[] = { "", ".", "..", "..." };

  // HUD window
  const auto& hudEntity = reg.ctx().get<HudEntity>();
  (void)hudEntity;  // entity cached

  ImGui::SetNextWindowPos({ 0.0f, 0.0f }, ImGuiCond_Always);
  ImGui::Begin("##hud", nullptr, kHUDFlags);

  // Using ImGui's built-in smoothed FPS (instead of frame_pacer.currentFps [might change back later])
  const int fi = frame.framesInFlight < 0 ? 0 : frame.framesInFlight > 3 ? 3 : frame.framesInFlight;
  ImGui::Text("FPS  %.0f %s", ImGui::GetIO().Framerate, kDots[fi]);

  ImGui::Text("FPS_LIM:");
  ImGui::SameLine(0.0f, 4.0f);
  ImGui::TextColored(fps_limited ? kGreen : kRed, fps_limited ? "ON" : "OFF");

  ImGui::Text("FSE:");
  ImGui::SameLine(0.0f, 4.0f);
  ImGui::TextColored(fullscreen ? kGreen : kRed, fullscreen ? "ON" : "OFF");

  ImGui::End();

  // Crosshair
  const ImGuiIO&  io   = ImGui::GetIO();
  ImDrawList*     fdl  = ImGui::GetForegroundDrawList();
  const ImVec2    c    = { io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f };
  constexpr ImU32 kCol = IM_COL32(0, 255, 160, 220);

  fdl->AddLine({ c.x - CROSSHAIRGAP - CROSSHAIRLENGTH, c.y }, { c.x - CROSSHAIRGAP, c.y }, kCol, CROSSHAIRTHICK);
  fdl->AddLine({ c.x + CROSSHAIRGAP, c.y }, { c.x + CROSSHAIRGAP + CROSSHAIRLENGTH, c.y }, kCol, CROSSHAIRTHICK);
  fdl->AddLine({ c.x, c.y - CROSSHAIRGAP - CROSSHAIRLENGTH }, { c.x, c.y - CROSSHAIRGAP }, kCol, CROSSHAIRTHICK);
  fdl->AddLine({ c.x, c.y + CROSSHAIRGAP }, { c.x, c.y + CROSSHAIRGAP + CROSSHAIRLENGTH }, kCol, CROSSHAIRTHICK);

  // Submit
  ImGui::Render();
  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), frame.frames[frame.frameIndex].commandBuffer);
}

void hud_shutdown(entt::registry& reg)
{
  auto& ctx = reg.ctx().get<VkContext>();
  shutdown(ctx.device);
}

}  // namespace sys
