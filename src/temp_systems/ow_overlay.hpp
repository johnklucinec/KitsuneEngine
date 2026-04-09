/*
 * ow_overlay.h — Non-interactive HUD overlay (ImGui + Vulkan 1.4 + SDL3 + Volk)
 *
 * DESCRIPTOR POOL REQUIREMENTS (your existing pool must include):
 *   - type:            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
 *   - descriptorCount: += 1  (for ImGui)
 *   - maxSets:         += 1  (for ImGui)
 *   - flags:           VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT
 *
 * USAGE:
 *   init(...)          — once, after swapchain creation
 *   begin_frame()      — top of render loop, before SDL_PollEvent
 *   build_ui(fd)       — after your draw calls, before vkCmdEndRendering
 *   record_draw(cmd)   — immediately after build_ui
 *   shutdown(device)   — after render loop, before vkDestroyDescriptorPool
 */
#pragma once
#include <volk/volk.h>
#include <imgui.h>
#include <imgui_impl_vulkan.h>
#include <imgui_impl_sdl3.h>
#include <SDL3/SDL.h>

#include "renderer/context.hpp"
#include "renderer/swapchain.hpp"
#include "renderer/pipeline.hpp"

namespace ow_overlay {

inline constexpr float crosshairLen   = 8.0f;
inline constexpr float crosshairGap   = 0.0f;
inline constexpr float crosshairThick = 1.5f;

struct FrameData
{
  float fps;
  int   frames_in_flight;
  bool  fps_limited;
  bool  fullscreen_exclusive;
};

inline void init(SDL_Window* window, const VkContext& ctx, const SwapchainState& sc, const PipelineState& ps) noexcept
{
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

  ImGui_ImplSDL3_InitForVulkan(window);

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
}


inline void shutdown(VkDevice device) noexcept
{
  vkDeviceWaitIdle(device);
  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplSDL3_Shutdown();
  ImGui::DestroyContext();
}

inline void begin_frame() noexcept
{
  ImGui_ImplSDL3_NewFrame();
  ImGui_ImplVulkan_NewFrame();
  ImGui::NewFrame();
}

inline void build_ui(const FrameData& d) noexcept
{
  constexpr ImGuiWindowFlags kHUDFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar
                                         | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize;

  ImGui::SetNextWindowPos({ 0.0f, 0.0f }, ImGuiCond_Always);
  ImGui::Begin("##hud", nullptr, kHUDFlags);

  static constexpr const char* kDots[] = { "", ".", "..", "..." };
  const int                    fi      = d.frames_in_flight < 0 ? 0 : d.frames_in_flight > 3 ? 3 : d.frames_in_flight;
  ImGui::Text("FPS  %.0f %s", d.fps, kDots[fi]);

  constexpr ImVec4 kGreen = { 0.20f, 0.85f, 0.20f, 1.0f };
  constexpr ImVec4 kRed   = { 0.85f, 0.20f, 0.20f, 1.0f };

  ImGui::Text("FPS_LIM:");
  ImGui::SameLine(0.0f, 4.0f);
  ImGui::TextColored(d.fps_limited ? kGreen : kRed, d.fps_limited ? "ON" : "OFF");

  ImGui::Text("FSE:");
  ImGui::SameLine(0.0f, 4.0f);
  ImGui::TextColored(d.fullscreen_exclusive ? kGreen : kRed, d.fullscreen_exclusive ? "ON" : "OFF");

  ImGui::End();

  const ImGuiIO& io  = ImGui::GetIO();
  ImDrawList*    fdl = ImGui::GetForegroundDrawList();
  const ImVec2   c   = { io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f };

  constexpr ImU32 kCol = IM_COL32(0, 255, 160, 220);

  fdl->AddLine({ c.x - crosshairGap - crosshairLen, c.y }, { c.x - crosshairGap, c.y }, kCol, crosshairThick);
  fdl->AddLine({ c.x + crosshairGap, c.y }, { c.x + crosshairGap + crosshairLen, c.y }, kCol, crosshairThick);
  fdl->AddLine({ c.x, c.y - crosshairGap - crosshairLen }, { c.x, c.y - crosshairGap }, kCol, crosshairThick);
  fdl->AddLine({ c.x, c.y + crosshairGap }, { c.x, c.y + crosshairGap + crosshairLen }, kCol, crosshairThick);
}

inline void record_draw(VkCommandBuffer cmd) noexcept
{
  ImGui::Render();
  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
}

}  // namespace ow_overlay
