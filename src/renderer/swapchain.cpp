#include "swapchain.hpp"
#include "common.hpp"
#include "renderer/context.hpp"
#include "renderer/resources.hpp"
#include "window.hpp"
#include "core/settings.hpp"
#include <cassert>
#include <entt/entity/registry.hpp>

namespace Renderer {

void initSwapchain(entt::registry& registry)
{
  auto& ctx        = registry.ctx().get<VkContext>();
  auto& sc         = registry.ctx().get<SwapchainState>();
  auto& window     = registry.ctx().get<WindowContext>();
  auto& settings   = registry.ctx().get<Settings>();
  bool  fullscreen = settings.fullscreen;

  // ========================================
  // Window and Surface
  if(fullscreen)  // TODO: should prob make this its own util function
  {
    SDL_DisplayID displayID = SDL_GetPrimaryDisplay();
    assert(displayID != 0);

    const SDL_DisplayMode* mode = SDL_GetCurrentDisplayMode(displayID);
    assert(mode);

    chk(SDL_SetWindowFullscreenMode(window, mode));  // NULL = borderless; mode = exclusive
    chk(SDL_SetWindowFullscreen(window, true));
    chk(SDL_SyncWindow(window));
  }

  VkSurfaceCapabilitiesKHR surfaceCaps{};
  chk(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx.physicalDevice, ctx.surface, &surfaceCaps));
  sc.extent = surfaceCaps.currentExtent;


  // ========================================
  // Swapchain
  sc.colorFormat = VK_FORMAT_B8G8R8A8_SRGB;

  uint32_t desiredImageCount = surfaceCaps.minImageCount + 1;
  if(surfaceCaps.maxImageCount > 0)
    desiredImageCount = std::min(desiredImageCount, surfaceCaps.maxImageCount);

  sc.swapchainCI = VkSwapchainCreateInfoKHR{
    .sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
    .surface          = ctx.surface,
    .minImageCount    = desiredImageCount,
    .imageFormat      = sc.colorFormat,
    .imageColorSpace  = VK_COLORSPACE_SRGB_NONLINEAR_KHR,
    .imageExtent      = surfaceCaps.currentExtent,
    .imageArrayLayers = 1,
    .imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    .preTransform     = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
    .compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
    .presentMode      = VK_PRESENT_MODE_IMMEDIATE_KHR,  // VK_PRESENT_MODE_FIFO_KHR = VSYNC | VK_PRESENT_MODE_IMMEDIATE_KHR = Unlocked
  };
  chk(vkCreateSwapchainKHR(ctx.device, &sc.swapchainCI, nullptr, &sc.swapchain));


  // ========================================
  // Images and Views

  // request images from the swapchain
  uint32_t imageCount{ 0 };
  chk(vkGetSwapchainImagesKHR(ctx.device, sc.swapchain, &imageCount, nullptr));
  sc.images.resize(imageCount);
  chk(vkGetSwapchainImagesKHR(ctx.device, sc.swapchain, &imageCount, sc.images.data()));

  // Create image views for each swapchain image
  sc.imageViews.resize(imageCount);
  for(auto i = 0; i < imageCount; i++)
  {
    VkImageViewCreateInfo viewCI{
      .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image    = sc.images[i],
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format   = sc.colorFormat,
      .subresourceRange{ .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 },
    };
    chk(vkCreateImageView(ctx.device, &viewCI, nullptr, &sc.imageViews[i]));
  }


  // ========================================
  // Depth Attachment
  std::vector<VkFormat> depthFormatList{ VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };
  for(VkFormat& format : depthFormatList)
  {
    VkFormatProperties2 formatProperties{ .sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2 };
    vkGetPhysicalDeviceFormatProperties2(ctx.physicalDevice, format, &formatProperties);
    if(formatProperties.formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
    {
      sc.depthFormat = format;
      break;
    }
  }
  assert(sc.depthFormat != VK_FORMAT_UNDEFINED);

  // Allocate depth buffer image and create image view
  sc.depthImageCI = VkImageCreateInfo{
    .sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
    .imageType     = VK_IMAGE_TYPE_2D,
    .format        = sc.depthFormat,
    .extent        = { surfaceCaps.currentExtent.width, surfaceCaps.currentExtent.height, 1 },
    .mipLevels     = 1,
    .arrayLayers   = 1,
    .samples       = VK_SAMPLE_COUNT_1_BIT,
    .tiling        = VK_IMAGE_TILING_OPTIMAL,
    .usage         = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
  };
  VmaAllocationCreateInfo allocCI{
    .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
    .usage = VMA_MEMORY_USAGE_AUTO,
  };
  chk(vmaCreateImage(ctx.allocator, &sc.depthImageCI, &allocCI, &sc.depthImage, &sc.depthAlloc, nullptr));
  VkImageViewCreateInfo depthViewCI{
    .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
    .image    = sc.depthImage,
    .viewType = VK_IMAGE_VIEW_TYPE_2D,
    .format   = sc.depthFormat,
    .subresourceRange{ .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT, .levelCount = 1, .layerCount = 1 }
  };
  chk(vkCreateImageView(ctx.device, &depthViewCI, nullptr, &sc.depthImageView));
}

void destroySwapchain(entt::registry& registry)
{
  auto& ctx = registry.ctx().get<VkContext>();
  auto& sc  = registry.ctx().get<SwapchainState>();

  vkDestroyImageView(ctx.device, sc.depthImageView, nullptr);
  vmaDestroyImage(ctx.allocator, sc.depthImage, sc.depthAlloc);

  for(VkImageView view : sc.imageViews)
    vkDestroyImageView(ctx.device, view, nullptr);
  sc.imageViews.clear();
  sc.images.clear();

  vkDestroySwapchainKHR(ctx.device, sc.swapchain, nullptr);

  sc.swapchain      = VK_NULL_HANDLE;
  sc.depthImage     = VK_NULL_HANDLE;
  sc.depthImageView = VK_NULL_HANDLE;
  sc.depthAlloc     = VK_NULL_HANDLE;
  sc.depthFormat    = VK_FORMAT_UNDEFINED;
}

void rebuildSwapchain(entt::registry& registry)
{
  auto& ctx = registry.ctx().get<VkContext>();
  auto& sc  = registry.ctx().get<SwapchainState>();

  // Wait until the GPU has completed all outstanding operations
  vkDeviceWaitIdle(ctx.device);

  VkSurfaceCapabilitiesKHR surfaceCaps{};
  chk(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx.physicalDevice, ctx.surface, &surfaceCaps));
  sc.extent = surfaceCaps.currentExtent;

  // Hold onto the old handle — pass it to the new swapchain so the driver can recycle resources
  sc.swapchainCI.oldSwapchain = sc.swapchain;
  sc.swapchainCI.imageExtent  = surfaceCaps.currentExtent;

  VkSwapchainKHR oldSwapchain = sc.swapchain;
  chk(vkCreateSwapchainKHR(ctx.device, &sc.swapchainCI, nullptr, &sc.swapchain));

  // Destroy old color views, then old swapchain
  for(VkImageView view : sc.imageViews)
    vkDestroyImageView(ctx.device, view, nullptr);
  sc.imageViews.clear();
  sc.images.clear();

  vkDestroySwapchainKHR(ctx.device, oldSwapchain, nullptr);
  oldSwapchain = VK_NULL_HANDLE;  // prevent accidental double-free

  // Acquire new swapchain images and create views
  uint32_t imageCount{ 0 };
  chk(vkGetSwapchainImagesKHR(ctx.device, sc.swapchain, &imageCount, nullptr));
  sc.images.resize(imageCount);
  chk(vkGetSwapchainImagesKHR(ctx.device, sc.swapchain, &imageCount, sc.images.data()));

  sc.imageViews.resize(imageCount);
  for(auto i = 0; i < imageCount; i++)
  {
    VkImageViewCreateInfo viewCI{
      .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image    = sc.images[i],
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format   = sc.colorFormat,
      .subresourceRange{ .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 },
    };
    chk(vkCreateImageView(ctx.device, &viewCI, nullptr, &sc.imageViews[i]));
  }

  vkDestroyImageView(ctx.device, sc.depthImageView, nullptr);
  vmaDestroyImage(ctx.allocator, sc.depthImage, sc.depthAlloc);

  sc.depthImageCI.extent = { surfaceCaps.currentExtent.width, surfaceCaps.currentExtent.height, 1 };

  VmaAllocationCreateInfo allocCI{
    .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
    .usage = VMA_MEMORY_USAGE_AUTO,
  };
  chk(vmaCreateImage(ctx.allocator, &sc.depthImageCI, &allocCI, &sc.depthImage, &sc.depthAlloc, nullptr));

  VkImageViewCreateInfo depthViewCI{
    .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
    .image    = sc.depthImage,
    .viewType = VK_IMAGE_VIEW_TYPE_2D,
    .format   = sc.depthFormat,
    .subresourceRange{ .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT, .levelCount = 1, .layerCount = 1 }
  };
  chk(vkCreateImageView(ctx.device, &depthViewCI, nullptr, &sc.depthImageView));
}

}  // namespace Renderer
