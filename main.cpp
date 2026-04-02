/* Copyright (c) 2025-2026, Johnny Klucinec
 * SPDX-License-Identifier: MIT */

#define VOLK_IMPLEMENTATION
#include <SDL3/SDL_video.h>
#include <cassert>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan.h>
#include <volk/volk.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vector>
#include <array>
#include <string>
#include <iostream>
#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include "slang/slang.h"
#include "slang/slang-com-ptr.h"
#include <ktx.h>
#include <ktxvulkan.h>
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <source_location>

#include "systems/ow_cam.hpp"
#include "systems/ow_move.hpp"
#include "systems/ow_overlay.hpp"
#include "systems/fps_limiter.hpp"

// ========================================
// Globals
struct Settings
{
  uint32_t device           = 0;       // GPU deviceIndex
  float    fov              = 103.0f;  // HFOV
  float    fps_max          = 120.0f;
  bool     fullscreen       = false;
  bool     reduce_buffering = false;
  float    sensitivity      = 2.70f;
  bool     show_ui          = true;
  bool     vsync            = false;
};

constexpr uint32_t MAX_FRAMES_IN_FLIGHT{ 2 };
uint32_t           framesInFlight{ 2 };
uint32_t           imageIndex{ 0 };
uint32_t           frameIndex{ 0 };

VkCommandPool         commandPool{ VK_NULL_HANDLE };
VkDevice              device{ VK_NULL_HANDLE };
VkImage               depthImage;
VkImageView           depthImageView;
VkDescriptorSetLayout descriptorSetLayoutTex{ VK_NULL_HANDLE };
VkDescriptorPool      descriptorPool{ VK_NULL_HANDLE };
VkDescriptorSet       descriptorSetTex{ VK_NULL_HANDLE };
VkInstance            instance{ VK_NULL_HANDLE };
VkPipeline            pipeline{ VK_NULL_HANDLE };
VkPipelineLayout      pipelineLayout{ VK_NULL_HANDLE };
VkQueue               queue{ VK_NULL_HANDLE };
VkSurfaceKHR          surface{ VK_NULL_HANDLE };
VkSwapchainKHR        swapchain{ VK_NULL_HANDLE };
bool                  updateSwapchain{ false };
bool                  updateFullscreen{ true };
VkBuffer              vBuffer{ VK_NULL_HANDLE };

VmaAllocator  allocator{ VK_NULL_HANDLE };
VmaAllocation depthImageAllocation;
VmaAllocation vBufferAllocation{ VK_NULL_HANDLE };

Slang::ComPtr<slang::IGlobalSession>              slangGlobalSession;
std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> commandBuffers;
std::array<VkFence, MAX_FRAMES_IN_FLIGHT>         fences;
std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT>     presentSemaphores;
std::vector<VkSemaphore>                          renderSemaphores;
std::vector<VkImage>                              swapchainImages;
std::vector<VkImageView>                          swapchainImageViews;

bool           isSprinting = false;
ow_cam::Camera camera{
  .pos         = { 0.0f, 0.0f, -6.0f },
  .sensitivity = 2.70f,
};
ow_move::Walker walker{};
glm::ivec2      windowSize{};

// ========================================
// Structs and Buffers
struct Vertex
{
  glm::vec3 pos;
  glm::vec3 normal;
  glm::vec2 uv;
};

struct ShaderData  // Look into scalarBlockLayout
{
  glm::mat4 projection;
  glm::mat4 view;
  glm::mat4 model[3];
  glm::vec4 lightPos{ 0.0f, -10.0f, 10.0f, 0.0f };
  uint32_t  selected{ 1 };
} shaderData{};

struct ShaderDataBuffer
{
  VmaAllocation     allocation{ VK_NULL_HANDLE };
  VmaAllocationInfo allocationInfo{};
  VkBuffer          buffer{ VK_NULL_HANDLE };
  VkDeviceAddress   deviceAddress{};
};
std::array<ShaderDataBuffer, MAX_FRAMES_IN_FLIGHT> shaderDataBuffers;

struct Texture
{
  VmaAllocation allocation{ VK_NULL_HANDLE };
  VkImage       image{ VK_NULL_HANDLE };
  VkImageView   view{ VK_NULL_HANDLE };
  VkSampler     sampler{ VK_NULL_HANDLE };
};
std::array<Texture, 3> textures{};

// Helper Function Prototypes
static inline void chk(VkResult result, const std::source_location& loc = std::source_location::current());
static inline void chkSwapchain(VkResult result, const std::source_location& loc = std::source_location::current());
static inline void chk(bool result, const std::source_location& loc = std::source_location::current());
Settings           parseArgs(int argc, char* argv[]);

int main(int argc, char* argv[])
{
  Settings settings = parseArgs(argc, argv);
  chk(SDL_Init(SDL_INIT_VIDEO));
  chk(SDL_Vulkan_LoadLibrary(NULL));
  volkInitialize();

  // ========================================
  // Instance Setup
  VkApplicationInfo appInfo{
    .sType            = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .pApplicationName = "Kitsune Engine",
    .apiVersion       = VK_API_VERSION_1_4,
  };

  uint32_t           instanceExtensionsCount{ 0 };
  char const* const* instanceExtensions{ SDL_Vulkan_GetInstanceExtensions(&instanceExtensionsCount) };

  VkInstanceCreateInfo instanceCI{
    .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    .pApplicationInfo        = &appInfo,
    .enabledExtensionCount   = instanceExtensionsCount,
    .ppEnabledExtensionNames = instanceExtensions,
  };
  chk(vkCreateInstance(&instanceCI, nullptr, &instance));
  volkLoadInstance(instance);

  // ========================================
  // Device Selection
  uint32_t deviceCount{ 0 };
  chk(vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr));
  std::vector<VkPhysicalDevice> devices(deviceCount);
  chk(vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data()));

  // Ensure valid deviceIndex
  assert(settings.device < deviceCount);

  // Output device name
  VkPhysicalDeviceProperties2 deviceProperties{
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
  };
  vkGetPhysicalDeviceProperties2(devices[settings.device], &deviceProperties);
  std::cout << "Selected device: " << deviceProperties.properties.deviceName << "\n";

  // ========================================
  // Find a Queue Family with Graphics Support
  uint32_t queueFamilyCount{ 0 };
  vkGetPhysicalDeviceQueueFamilyProperties(devices[settings.device], &queueFamilyCount, nullptr);
  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(devices[settings.device], &queueFamilyCount, queueFamilies.data());
  uint32_t queueFamily{ 0 };
  for(size_t i = 0; i < queueFamilies.size(); i++)
  {
    if(queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
    {
      queueFamily = i;
      break;
    }
  }
  chk(SDL_Vulkan_GetPresentationSupport(instance, devices[settings.device], queueFamily));

  // ========================================
  // Logical Device Setup
  const float             qfpriorities{ 1.0f };
  VkDeviceQueueCreateInfo queueCI{
    .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
    .queueFamilyIndex = queueFamily,
    .queueCount       = 1,
    .pQueuePriorities = &qfpriorities,
  };
  VkPhysicalDeviceVulkan12Features enabledVk12Features{
    .sType                                     = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
    .descriptorIndexing                        = true,
    .shaderSampledImageArrayNonUniformIndexing = true,
    .descriptorBindingVariableDescriptorCount  = true,
    .runtimeDescriptorArray                    = true,
    .bufferDeviceAddress                       = true,
  };
  VkPhysicalDeviceVulkan13Features enabledVk13Features{
    .sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
    .pNext            = &enabledVk12Features,
    .synchronization2 = true,
    .dynamicRendering = true,
  };
  const VkPhysicalDeviceFeatures enabledVk10Features{
    .samplerAnisotropy = VK_TRUE,
  };
  const std::vector<const char*> deviceExtensions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };

  VkDeviceCreateInfo deviceCI{
    .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    .pNext                   = &enabledVk13Features,
    .queueCreateInfoCount    = 1,
    .pQueueCreateInfos       = &queueCI,
    .enabledExtensionCount   = static_cast<uint32_t>(deviceExtensions.size()),
    .ppEnabledExtensionNames = deviceExtensions.data(),
    .pEnabledFeatures        = &enabledVk10Features,
  };
  chk(vkCreateDevice(devices[settings.device], &deviceCI, nullptr, &device));
  vkGetDeviceQueue(device, queueFamily, 0, &queue);  // request queue from device

  // ========================================
  // Setting up VMA
  VmaVulkanFunctions vkFunctions{
    .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
    .vkGetDeviceProcAddr   = vkGetDeviceProcAddr,
    .vkCreateImage         = vkCreateImage,
  };
  VmaAllocatorCreateInfo allocatorCI{
    .flags            = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
    .physicalDevice   = devices[settings.device],
    .device           = device,
    .pVulkanFunctions = &vkFunctions,
    .instance         = instance,
  };
  chk(vmaCreateAllocator(&allocatorCI, &allocator));

  // ========================================
  // Window and Surface
  SDL_Window* window = SDL_CreateWindow("KitsuneEngine", 1600u, 900u, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
  assert(window);

  if(settings.fullscreen)
  {
    SDL_DisplayID displayID = SDL_GetPrimaryDisplay();
    assert(displayID != 0);

    const SDL_DisplayMode* mode = SDL_GetCurrentDisplayMode(displayID);
    assert(mode);

    chk(SDL_SetWindowFullscreenMode(window, mode));  // NULL = borderless; mode = exclusive
    chk(SDL_SetWindowFullscreen(window, true));
    chk(SDL_SyncWindow(window));
  }

  chk(SDL_Vulkan_CreateSurface(window, instance, nullptr, &surface));
  chk(SDL_GetWindowSize(window, &windowSize.x, &windowSize.y));  // now returns fullscreen dims if applicable
  SDL_SetWindowRelativeMouseMode(window, true);
  VkSurfaceCapabilitiesKHR surfaceCaps{};
  chk(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(devices[settings.device], surface, &surfaceCaps));

  // ========================================
  // Swapchain
  const VkFormat           imageFormat{ VK_FORMAT_B8G8R8A8_SRGB };
  VkSwapchainCreateInfoKHR swapchainCI{
    .sType           = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
    .surface         = surface,
    .minImageCount   = surfaceCaps.minImageCount,
    .imageFormat     = imageFormat,
    .imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR,
    .imageExtent{ .width = surfaceCaps.currentExtent.width, .height = surfaceCaps.currentExtent.height },
    .imageArrayLayers = 1,
    .imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    .preTransform     = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
    .compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
    .presentMode      = VK_PRESENT_MODE_IMMEDIATE_KHR, // VK_PRESENT_MODE_FIFO_KHR = VSYNC | VK_PRESENT_MODE_IMMEDIATE_KHR = Unlocked
  };
  chk(vkCreateSwapchainKHR(device, &swapchainCI, nullptr, &swapchain));

  // request images from the swapchain
  uint32_t imageCount{ 0 };
  chk(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr));
  swapchainImages.resize(imageCount);
  chk(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data()));
  swapchainImageViews.resize(imageCount);

  // Create image views for each swapchain image
  for(auto i = 0; i < imageCount; i++)
  {
    VkImageViewCreateInfo viewCI{
      .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image    = swapchainImages[i],
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format   = imageFormat,
      .subresourceRange{ .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 },
    };
    chk(vkCreateImageView(device, &viewCI, nullptr, &swapchainImageViews[i]));
  }

  // ========================================
  // Depth Attachment
  std::vector<VkFormat> depthFormatList{ VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };
  VkFormat              depthFormat{ VK_FORMAT_UNDEFINED };
  for(VkFormat& format : depthFormatList)
  {
    VkFormatProperties2 formatProperties{ .sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2 };
    vkGetPhysicalDeviceFormatProperties2(devices[settings.device], format, &formatProperties);
    if(formatProperties.formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
    {
      depthFormat = format;
      break;
    }
  }
  assert(depthFormat != VK_FORMAT_UNDEFINED);

  // Allocate depth buffer image and create image view
  VkImageCreateInfo depthImageCI{
    .sType     = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
    .imageType = VK_IMAGE_TYPE_2D,
    .format    = depthFormat,
    .extent{ .width = static_cast<uint32_t>(windowSize.x), .height = static_cast<uint32_t>(windowSize.y), .depth = 1 },
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
  chk(vmaCreateImage(allocator, &depthImageCI, &allocCI, &depthImage, &depthImageAllocation, nullptr));
  VkImageViewCreateInfo depthViewCI{
    .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
    .image    = depthImage,
    .viewType = VK_IMAGE_VIEW_TYPE_2D,
    .format   = depthFormat,
    .subresourceRange{ .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT, .levelCount = 1, .layerCount = 1 }
  };
  chk(vkCreateImageView(device, &depthViewCI, nullptr, &depthImageView));

  // ========================================
  // Loading Meshes
  tinyobj::attrib_t                attrib;
  std::vector<tinyobj::shape_t>    shapes;
  std::vector<tinyobj::material_t> materials;
  chk(tinyobj::LoadObj(&attrib, &shapes, &materials, nullptr, nullptr, "assets/suzanne.obj"));
  const VkDeviceSize    indexCount{ shapes[0].mesh.indices.size() };
  std::vector<Vertex>   vertices{};
  std::vector<uint16_t> indices{};

  // Load vertex and index data
  for(auto& index : shapes[0].mesh.indices)
  {
    Vertex v{
      .pos    = { attrib.vertices[index.vertex_index * 3], -attrib.vertices[index.vertex_index * 3 + 1], attrib.vertices[index.vertex_index * 3 + 2] },
      .normal = { attrib.normals[index.normal_index * 3], -attrib.normals[index.normal_index * 3 + 1], attrib.normals[index.normal_index * 3 + 2] },
      .uv     = { attrib.texcoords[index.texcoord_index * 2], 1.0 - attrib.texcoords[index.texcoord_index * 2 + 1] }
    };
    vertices.push_back(v);
    indices.push_back(indices.size());
  }

  // Allocate vertex + index buffer
  VkDeviceSize       vBufSize{ sizeof(Vertex) * vertices.size() };
  VkDeviceSize       iBufSize{ sizeof(uint16_t) * indices.size() };
  VkBufferCreateInfo bufferCI{
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .size  = vBufSize + iBufSize,
    .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
  };

  // Persistent Mapping
  VmaAllocationCreateInfo vBufferAllocCI{
    // NOTE: without ReBAR, host-visible VRAM is limited to 256MB. Exhausting it falls back to non-host-visible DEVICE_LOCAL via ALLOW_TRANSFER_INSTEAD
    .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT
             | VMA_ALLOCATION_CREATE_MAPPED_BIT,  // fallback fires once the 256MB BAR is exhausted, which is common on non-ReBAR discrete GPUs
    .usage = VMA_MEMORY_USAGE_AUTO,
  };
  VmaAllocationInfo vBufferAllocInfo{};
  chk(vmaCreateBuffer(allocator, &bufferCI, &vBufferAllocCI, &vBuffer, &vBufferAllocation, &vBufferAllocInfo));

  // Write directly into VRAM via persistently mapped ReBAR memory
  memcpy(vBufferAllocInfo.pMappedData, vertices.data(), vBufSize);
  memcpy(((char*)vBufferAllocInfo.pMappedData) + vBufSize, indices.data(), iBufSize);

  // ========================================
  // Shader Data Buffers
  for(auto i = 0; i < framesInFlight; i++)  // One per max frame in flight
  {
    VkBufferCreateInfo uBufferCI{
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size  = sizeof(ShaderData),
      .usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,  // Buffer accessible via raw pointer in shader
    };
    VmaAllocationCreateInfo uBufferAllocCI{
      .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT
               | VMA_ALLOCATION_CREATE_MAPPED_BIT,  // Persistently mapped; CPU/GPU accessible
      .usage = VMA_MEMORY_USAGE_AUTO,
    };
    chk(vmaCreateBuffer(allocator, &uBufferCI, &uBufferAllocCI, &shaderDataBuffers[i].buffer, &shaderDataBuffers[i].allocation, &shaderDataBuffers[i].allocationInfo));

    // Grab and store buffer's device address
    VkBufferDeviceAddressInfo uBufferBdaInfo{ .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .buffer = shaderDataBuffers[i].buffer };
    shaderDataBuffers[i].deviceAddress = vkGetBufferDeviceAddress(device, &uBufferBdaInfo);
  }

  // ========================================
  // Synchronization Objects
  VkSemaphoreCreateInfo semaphoreCI{
    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
  };
  VkFenceCreateInfo fenceCI{
    .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    .flags = VK_FENCE_CREATE_SIGNALED_BIT,
  };
  for(auto i = 0; i < framesInFlight; i++)  // One fence per frame in flight
  {
    chk(vkCreateFence(device, &fenceCI, nullptr, &fences[i]));
    chk(vkCreateSemaphore(device, &semaphoreCI, nullptr, &presentSemaphores[i]));
  }
  renderSemaphores.resize(swapchainImages.size());
  for(auto& semaphore : renderSemaphores)  // One semephore per swapchain image
  {
    chk(vkCreateSemaphore(device, &semaphoreCI, nullptr, &semaphore));
  }

  // ========================================
  // Command Buffers (pools)
  VkCommandPoolCreateInfo commandPoolCI{
    .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
    .queueFamilyIndex = queueFamily,
  };
  chk(vkCreateCommandPool(device, &commandPoolCI, nullptr, &commandPool));
  VkCommandBufferAllocateInfo cbAllocCI{
    .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .commandPool        = commandPool,
    .commandBufferCount = framesInFlight,  // One command buffer per frame in flight
  };
  chk(vkAllocateCommandBuffers(device, &cbAllocCI, commandBuffers.data()));

  // ========================================
  // Loading Textures
  std::vector<VkDescriptorImageInfo> textureDescriptors{};
  for(auto i = 0; i < textures.size(); i++)
  {
    ktxTexture* ktxTexture{ nullptr };
    std::string filename = "assets/suzanne" + std::to_string(i) + ".ktx";
    ktxTexture_CreateFromNamedFile(filename.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTexture);

    // Creating the image (object)
    VkImageCreateInfo texImgCI{
      .sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .imageType     = VK_IMAGE_TYPE_2D,
      .format        = ktxTexture_GetVkFormat(ktxTexture),
      .extent        = { .width = ktxTexture->baseWidth, .height = ktxTexture->baseHeight, .depth = 1 },
      .mipLevels     = ktxTexture->numLevels,
      .arrayLayers   = 1,
      .samples       = VK_SAMPLE_COUNT_1_BIT,
      .tiling        = VK_IMAGE_TILING_OPTIMAL,
      .usage         = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    VmaAllocationCreateInfo texImageAllocCI{ .usage = VMA_MEMORY_USAGE_AUTO };
    chk(vmaCreateImage(allocator, &texImgCI, &texImageAllocCI, &textures[i].image, &textures[i].allocation, nullptr));

    // Create the image view
    VkImageViewCreateInfo texVewCI{
      .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image            = textures[i].image,
      .viewType         = VK_IMAGE_VIEW_TYPE_2D,
      .format           = texImgCI.format,
      .subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = ktxTexture->numLevels, .layerCount = 1 }
    };
    chk(vkCreateImageView(device, &texVewCI, nullptr, &textures[i].view));

    // ========================================
    // Upload
    VkBuffer           imgSrcBuffer{};
    VmaAllocation      imgSrcAllocation{};
    VkBufferCreateInfo imgSrcBufferCI{
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size  = (uint32_t)ktxTexture->dataSize,
      .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,  // Temporary source for a buffer-to-image copy
    };
    VmaAllocationCreateInfo imgSrcAllocCI{
      .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
      .usage = VMA_MEMORY_USAGE_AUTO,
    };
    VmaAllocationInfo imgSrcAllocInfo{};
    chk(vmaCreateBuffer(allocator, &imgSrcBufferCI, &imgSrcAllocCI, &imgSrcBuffer, &imgSrcAllocation, &imgSrcAllocInfo));
    memcpy(imgSrcAllocInfo.pMappedData, ktxTexture->pData, ktxTexture->dataSize);  // Actually move image data now

    // Copy image data to the optimal tiled image on the GPU
    VkFenceCreateInfo fenceOneTimeCI{
      .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    };
    VkFence fenceOneTime{};
    chk(vkCreateFence(device, &fenceOneTimeCI, nullptr, &fenceOneTime));
    VkCommandBuffer             cbOneTime{};
    VkCommandBufferAllocateInfo cbOneTimeAI{
      .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool        = commandPool,
      .commandBufferCount = 1,
    };
    chk(vkAllocateCommandBuffers(device, &cbOneTimeAI, &cbOneTime));

    // Start recording the commands required to get the image to its destination
    VkCommandBufferBeginInfo cbOneTimeBI{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    chk(vkBeginCommandBuffer(cbOneTime, &cbOneTimeBI));

    // Transition mip levels from initial undefined layout to a transferable layout
    VkImageMemoryBarrier2 barrierTexImage{
      .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
      .srcStageMask     = VK_PIPELINE_STAGE_2_NONE,
      .srcAccessMask    = VK_ACCESS_2_NONE,
      .dstStageMask     = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
      .dstAccessMask    = VK_ACCESS_2_TRANSFER_WRITE_BIT,
      .oldLayout        = VK_IMAGE_LAYOUT_UNDEFINED,
      .newLayout        = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, // The good layout
      .image            = textures[i].image,
      .subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = ktxTexture->numLevels, .layerCount = 1 }
    };
    VkDependencyInfo barrierTexInfo{
      .sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
      .imageMemoryBarrierCount = 1,
      .pImageMemoryBarriers    = &barrierTexImage,
    };
    vkCmdPipelineBarrier2(cbOneTime, &barrierTexInfo);
    std::vector<VkBufferImageCopy> copyRegions{};
    for(auto j = 0; j < ktxTexture->numLevels; j++)
    {
      ktx_size_t     mipOffset{ 0 };
      KTX_error_code ret = ktxTexture_GetImageOffset(ktxTexture, j, 0, 0, &mipOffset);
      copyRegions.push_back({
          .bufferOffset = mipOffset,  // clang-format off
          .imageSubresource{ .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = (uint32_t)j, .layerCount = 1 },
          .imageExtent{ .width = ktxTexture->baseWidth >> j, .height = ktxTexture->baseHeight >> j, .depth = 1 },
      });  // clang-format on
    }

    // Copy mip levels from temp buffer
    vkCmdCopyBufferToImage(cbOneTime, imgSrcBuffer, textures[i].image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<uint32_t>(copyRegions.size()),
                           copyRegions.data());

    // Transition mip levels from transfer destination to shader readable layout
    VkImageMemoryBarrier2 barrierTexRead{
      .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
      .srcStageMask     = VK_PIPELINE_STAGE_TRANSFER_BIT,
      .srcAccessMask    = VK_ACCESS_TRANSFER_WRITE_BIT,
      .dstStageMask     = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
      .dstAccessMask    = VK_ACCESS_SHADER_READ_BIT,
      .oldLayout        = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      .newLayout        = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,  // Yay, shaders can read
      .image            = textures[i].image,
      .subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = ktxTexture->numLevels, .layerCount = 1 }
    };
    barrierTexInfo.pImageMemoryBarriers = &barrierTexRead;
    vkCmdPipelineBarrier2(cbOneTime, &barrierTexInfo);
    chk(vkEndCommandBuffer(cbOneTime));
    VkSubmitInfo oneTimeSI{
      .sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .commandBufferCount = 1,
      .pCommandBuffers    = &cbOneTime,
    };
    chk(vkQueueSubmit(queue, 1, &oneTimeSI, fenceOneTime));
    chk(vkWaitForFences(device, 1, &fenceOneTime, VK_TRUE, UINT64_MAX));
    vkDestroyFence(device, fenceOneTime, nullptr);
    vmaDestroyBuffer(allocator, imgSrcBuffer, imgSrcAllocation);

    // ========================================
    // Sample Texture in Shader
    VkSamplerCreateInfo samplerCI{
      .sType            = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
      .magFilter        = VK_FILTER_LINEAR,
      .minFilter        = VK_FILTER_LINEAR,
      .mipmapMode       = VK_SAMPLER_MIPMAP_MODE_LINEAR,
      .anisotropyEnable = VK_TRUE,
      .maxAnisotropy    = 8.0f,  // 8 is a widely supported value for max anisotropy
      .maxLod           = (float)ktxTexture->numLevels,
    };
    chk(vkCreateSampler(device, &samplerCI, nullptr, &textures[i].sampler));

    // Clean up and store descriptor info for texture to use later
    ktxTexture_Destroy(ktxTexture);
    textureDescriptors.push_back({
        .sampler     = textures[i].sampler,
        .imageView   = textures[i].view,
        .imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
    });
  }

  // ========================================
  // Descriptor Indexing
  VkDescriptorBindingFlags descVariableFlag{ VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT };

  // Only using bindings for images
  VkDescriptorSetLayoutBindingFlagsCreateInfo descBindingFlags{
    .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
    .bindingCount  = 1,
    .pBindingFlags = &descVariableFlag,
  };
  VkDescriptorSetLayoutBinding descLayoutBindingTex{
    .descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,  // Will be combining textures and samplers
    .descriptorCount = static_cast<uint32_t>(textures.size()),
    .stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT,
  };
  VkDescriptorSetLayoutCreateInfo descLayoutTexCI{
    .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    .pNext        = &descBindingFlags,
    .bindingCount = 1,
    .pBindings    = &descLayoutBindingTex,
  };
  chk(vkCreateDescriptorSetLayout(device, &descLayoutTexCI, nullptr, &descriptorSetLayoutTex));  // Defines the interface

  // Allocate descriptors
  VkDescriptorPoolSize poolSize{
    .type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    .descriptorCount = static_cast<uint32_t>(textures.size()) + 1,  // As many descriptors for images + samplers per texture + 1 for ImGui
  };
  VkDescriptorPoolCreateInfo descPoolCI{
    .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
    .flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,  // for ImGui
    .maxSets       = 2,                                                  // how mant descriptor sets, not descriptors + 1 for ImGui
    .poolSizeCount = 1,
    .pPoolSizes    = &poolSize,  // If wrong, allocations beyound requested counts will fail
  };
  chk(vkCreateDescriptorPool(device, &descPoolCI, nullptr, &descriptorPool));

  // Initialize the ImGui overlay
  ow_overlay::init(window, instance, devices[settings.device], device, queue, queueFamily, static_cast<uint32_t>(swapchainImages.size()), imageFormat,
                   descriptorPool, surfaceCaps.minImageCount);

  uint32_t variableDescCount{ static_cast<uint32_t>(textures.size()) };
  // Allocate descriptor sets from that pool
  VkDescriptorSetVariableDescriptorCountAllocateInfo variableDescCountAI{
    .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT,
    .descriptorSetCount = 1,
    .pDescriptorCounts  = &variableDescCount,
  };
  VkDescriptorSetAllocateInfo texDescSetAlloc{
    .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    .pNext              = &variableDescCountAI,
    .descriptorPool     = descriptorPool,
    .descriptorSetCount = 1,
    .pSetLayouts        = &descriptorSetLayoutTex,  // Interface to our shaders
  };
  chk(vkAllocateDescriptorSets(device, &texDescSetAlloc, &descriptorSetTex));  // Contains the actual descriptor data

  // Put data in descriptor set so we can access it in the shader
  VkWriteDescriptorSet writeDescSet{
    .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
    .dstSet          = descriptorSetTex,
    .dstBinding      = 0,
    .descriptorCount = static_cast<uint32_t>(textureDescriptors.size()),
    .descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    .pImageInfo      = textureDescriptors.data(),
  };
  vkUpdateDescriptorSets(device, 1, &writeDescSet, 0, nullptr);  // put info in first binding slot of the descriptor set

  // ========================================
  // Loading Shaders
  // Initialize Slang shader compiler
  slang::createGlobalSession(slangGlobalSession.writeRef());
  auto slangTargets{
    std::to_array<slang::TargetDesc>({ {
        .format  = SLANG_SPIRV,
        .profile = slangGlobalSession->findProfile("spirv_1_4"),
    } })
  };
  auto slangOptions{
    std::to_array<slang::CompilerOptionEntry>({ {
        slang::CompilerOptionName::EmitSpirvDirectly,
        { slang::CompilerOptionValueKind::Int, 1 },
    } })
  };
  slang::SessionDesc slangSessionDesc{
    .targets                  = slangTargets.data(),
    .targetCount              = SlangInt(slangTargets.size()),
    .defaultMatrixLayoutMode  = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR,  // Matches the layout of GLM
    .compilerOptionEntries    = slangOptions.data(),
    .compilerOptionEntryCount = uint32_t(slangOptions.size()),
  };
  Slang::ComPtr<slang::ISession> slangSession;
  slangGlobalSession->createSession(slangSessionDesc, slangSession.writeRef());

  // Load the Shader
  Slang::ComPtr<slang::IModule> slangModule{ slangSession->loadModuleFromSource("triangle", "assets/shader.slang", nullptr, nullptr) };
  Slang::ComPtr<ISlangBlob>     spirv;
  slangModule->getTargetCode(0, spirv.writeRef());

  // Create shader module for use in graphics pipeline
  VkShaderModuleCreateInfo shaderModuleCI{
    .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    .codeSize = spirv->getBufferSize(),
    .pCode    = (uint32_t*)spirv->getBufferPointer(),
  };
  VkShaderModule shaderModule{};
  chk(vkCreateShaderModule(device, &shaderModuleCI, nullptr, &shaderModule));

  // ========================================
  // Graphics Pipeline
  VkPushConstantRange pushConstantRange{
    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
    .size       = sizeof(VkDeviceAddress),
  };
  VkPipelineLayoutCreateInfo pipelineLayoutCI{
    .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .setLayoutCount         = 1,
    .pSetLayouts            = &descriptorSetLayoutTex,  // Interface to shader resources
    .pushConstantRangeCount = 1,
    .pPushConstantRanges    = &pushConstantRange,
  };
  chk(vkCreatePipelineLayout(device, &pipelineLayoutCI, nullptr, &pipelineLayout));  // Creates pipeline layout

  // Define basic vertex structures in Vulkan terms
  std::vector<VkPipelineShaderStageCreateInfo> shaderStages{
    { .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .stage = VK_SHADER_STAGE_VERTEX_BIT, .module = shaderModule, .pName = "main" },
    { .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .stage = VK_SHADER_STAGE_FRAGMENT_BIT, .module = shaderModule, .pName = "main" }
  };
  VkVertexInputBindingDescription                vertexBinding{ .binding = 0, .stride = sizeof(Vertex), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX };
  std::vector<VkVertexInputAttributeDescription> vertexAttributes{
    { .location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT },
    { .location = 1, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex, normal) },
    { .location = 2, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(Vertex, uv) },
  };

  // Setup all the pipeline states
  VkPipelineVertexInputStateCreateInfo vertexInputState{
    .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    .vertexBindingDescriptionCount   = 1,
    .pVertexBindingDescriptions      = &vertexBinding,
    .vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributes.size()),
    .pVertexAttributeDescriptions    = vertexAttributes.data(),
  };
  VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{
    .sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
    .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,  // we be rendering a list of triangles
  };
  std::vector<VkDynamicState>      dynamicStates{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
  VkPipelineDynamicStateCreateInfo dynamicState{
    .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
    .dynamicStateCount = 2,
    .pDynamicStates    = dynamicStates.data(),
  };
  VkPipelineViewportStateCreateInfo viewportState{
    .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
    .viewportCount = 1,
    .scissorCount  = 1,
  };
  VkPipelineRasterizationStateCreateInfo rasterizationState{
    .sType     = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
    .lineWidth = 1.0f,  // Not used (default state)
  };
  VkPipelineMultisampleStateCreateInfo multisampleState{
    .sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
    .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,  // Not used (default state)
  };
  VkPipelineDepthStencilStateCreateInfo depthStencilState{
    .sType            = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
    .depthTestEnable  = VK_TRUE,
    .depthWriteEnable = VK_TRUE,
    .depthCompareOp   = VK_COMPARE_OP_LESS_OR_EQUAL,
  };
  VkPipelineColorBlendAttachmentState blendAttachment{
    .colorWriteMask = 0xF,  // Not used (default state)
  };
  VkPipelineColorBlendStateCreateInfo colorBlendState{
    .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
    .attachmentCount = 1,
    .pAttachments    = &blendAttachment,
  };
  VkPipelineRenderingCreateInfo renderingCI{
    .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,  // We are using dynamic rendering
    .colorAttachmentCount    = 1,
    .pColorAttachmentFormats = &imageFormat,
    .depthAttachmentFormat   = depthFormat,
  };

  // Create the Graphics Pipeline
  VkGraphicsPipelineCreateInfo pipelineCI{
    .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
    .pNext               = &renderingCI,
    .stageCount          = 2,
    .pStages             = shaderStages.data(),
    .pVertexInputState   = &vertexInputState,
    .pInputAssemblyState = &inputAssemblyState,
    .pViewportState      = &viewportState,
    .pRasterizationState = &rasterizationState,
    .pMultisampleState   = &multisampleState,
    .pDepthStencilState  = &depthStencilState,
    .pColorBlendState    = &colorBlendState,
    .pDynamicState       = &dynamicState,
    .layout              = pipelineLayout,
  };
  chk(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &pipeline));

  // ========================================
  // Render Loop
  fps::Limiter limiter(settings.fps_max);
  bool         quit{ false };

  while(!quit)
  {
    // ==== Wait on Fence (sync) ====
    chk(vkWaitForFences(device, 1, &fences[frameIndex], true, UINT64_MAX));  // Wait for last frame GPU is working on
    chk(vkResetFences(device, 1, &fences[frameIndex]));                      // Reset for next submission

    // Setup frame pacing
    limiter.wait();
    float elapsedTime = limiter.deltaTime();
    float fps         = limiter.currentFPS();

    if(settings.show_ui)
      ow_overlay::begin_frame();  // begin frame for ImGui

    // ==== Aquire Next Image ====
    chkSwapchain(vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, presentSemaphores[frameIndex], VK_NULL_HANDLE, &imageIndex));

    // === Polling Events ===
    constexpr int maxShaderIndex = 2;
    static bool   keyState[SDL_SCANCODE_COUNT]{};

    SDL_Event event;
    while(SDL_PollEvent(&event))
    {
      switch(event.type)
      {
        case SDL_EVENT_QUIT:
          quit = true;
          break;

        case SDL_EVENT_MOUSE_MOTION:
          camera.apply_mouse_delta(event.motion.xrel, event.motion.yrel);
          break;

        case SDL_EVENT_KEY_DOWN:
          keyState[event.key.scancode] = true;

          switch(event.key.key)
          {
            case SDLK_ESCAPE:
              quit = true;
              break;
            case SDLK_LEFT:
              shaderData.selected = (shaderData.selected > 0) ? shaderData.selected - 1 : maxShaderIndex;
              break;
            case SDLK_RIGHT:
              shaderData.selected = (shaderData.selected < maxShaderIndex) ? shaderData.selected + 1 : 0;
              break;
            default:
              break;
          }

          if(event.key.mod & SDL_KMOD_LALT)
          {
            switch(event.key.key)
            {
              case SDLK_RETURN:
                settings.fullscreen = !settings.fullscreen;
                updateFullscreen    = true;
                break;
              case SDLK_Z:
                settings.show_ui = !settings.show_ui;
                break;
            }
          }
          break;

        case SDL_EVENT_KEY_UP:
          keyState[event.key.scancode] = false;
          break;

        case SDL_EVENT_WINDOW_RESIZED:
          updateSwapchain = true;
          break;

        default:
          break;
      }
    }

    float fwd = 0.0f, side = 0.0f;
    if(keyState[SDL_SCANCODE_W])
      fwd += 1.0f;
    if(keyState[SDL_SCANCODE_S])
      fwd -= 1.0f;
    if(keyState[SDL_SCANCODE_A])
      side += 1.0f;
    if(keyState[SDL_SCANCODE_D])
      side -= 1.0f;
    isSprinting = keyState[SDL_SCANCODE_LSHIFT];

    walker.update(fwd, side, isSprinting);                                            // Update movement
    walker.integrate_world(camera.pos.x, camera.pos.z, camera.yaw_f(), elapsedTime);  // Update velocity, then integrate

    // ==== Update Shader Data ====
    float vfov            = ow_cam::vfov_from_hfov_cfg(settings.fov);
    shaderData.projection = glm::perspective(vfov, (float)windowSize.x / (float)windowSize.y, 0.1f, 32.0f);

    // View matrix built from OW camera yaw/pitch
    glm::mat4 rotYaw   = glm::rotate(glm::mat4(1.0f), camera.yaw_f(), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 rotPitch = glm::rotate(glm::mat4(1.0f), -camera.pitch_f(), glm::vec3(1.0f, 0.0f, 0.0f));
    shaderData.view    = rotPitch * rotYaw * glm::translate(glm::mat4(1.0f), camera.pos);

    for(auto i = 0; i < 3; i++)
    {
      auto instancePos    = glm::vec3((float)(i - 1) * 3.0f, 0.0f, 0.0f);
      shaderData.model[i] = glm::translate(glm::mat4(1.0f), instancePos);  // static, no rotation
    }
    memcpy(shaderDataBuffers[frameIndex].allocationInfo.pMappedData, &shaderData, sizeof(ShaderData));

    // ==== Record Command Buffer (build command buffer) ====
    auto cb = commandBuffers[frameIndex];
    chk(vkResetCommandBuffer(cb, 0));  // Reset command buffer
    VkCommandBufferBeginInfo cbBI{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    chk(vkBeginCommandBuffer(cb, &cbBI));  // Start recording to cb

    // Issue layout transitions for swapchain image and depth image
    std::array<VkImageMemoryBarrier2, 2> outputBarriers{
      VkImageMemoryBarrier2{
          .sType         = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
          .srcStageMask  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,  // pipeline stage(s) to wait on
          .srcAccessMask = 0,                                                // memory writes to make available to the GPU
          .dstStageMask  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,  // where and what those writes must be visible to
          .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
          .oldLayout     = VK_IMAGE_LAYOUT_UNDEFINED,           // dont need previous contents
          .newLayout     = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,  // use as color attachment for rendering
          .image         = swapchainImages[imageIndex],
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
          .image         = depthImage,
          .subresourceRange{ .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, .levelCount = 1, .layerCount = 1 },
      }
    };
    VkDependencyInfo barrierDependencyInfo{
      .sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
      .imageMemoryBarrierCount = 2,
      .pImageMemoryBarriers    = outputBarriers.data(),
    };
    vkCmdPipelineBarrier2(cb, &barrierDependencyInfo);  // Insert those two barriers into the current command buffer

    // Define how attachments are used: Dynamic Rendering
    VkRenderingAttachmentInfo colorAttachmentInfo{
      .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView   = swapchainImageViews[imageIndex],
      .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
      .loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp     = VK_ATTACHMENT_STORE_OP_STORE,
      .clearValue{ .color{ 0.0f, 0.0f, 0.0f, 1.0f } },
    };
    VkRenderingAttachmentInfo depthAttachmentInfo{
      .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView   = depthImageView,
      .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
      .loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp     = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .clearValue  = { .depthStencil = { 1.0f, 0 } },
    };

    // Start dynamic render pass
    VkRenderingInfo renderingInfo{
      .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
      .renderArea{ .extent{ .width = static_cast<uint32_t>(windowSize.x), .height = static_cast<uint32_t>(windowSize.y) } },
      .layerCount           = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments    = &colorAttachmentInfo,
      .pDepthAttachment     = &depthAttachmentInfo,
    };
    vkCmdBeginRendering(cb, &renderingInfo);

    // Start recording GPU commands [vkCmd*]
    VkViewport vp{
      .width    = static_cast<float>(windowSize.x),
      .height   = static_cast<float>(windowSize.y),
      .minDepth = 0.0f,
      .maxDepth = 1.0f,
    };
    vkCmdSetViewport(cb, 0, 1, &vp);
    VkRect2D scissor{
      .extent{
          .width  = static_cast<uint32_t>(windowSize.x),
          .height = static_cast<uint32_t>(windowSize.y),
      }
    };

    // Bind resources involved in rendering the 3D objects
    vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdSetScissor(cb, 0, 1, &scissor);
    vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSetTex, 0, nullptr);
    VkDeviceSize vOffset{ 0 };
    vkCmdBindVertexBuffers(cb, 0, 1, &vBuffer, &vOffset);
    vkCmdBindIndexBuffer(cb, vBuffer, vBufSize, VK_INDEX_TYPE_UINT16);

    // Pass the address of the current frame's shader data buffer via a push constant to the shaders
    vkCmdPushConstants(cb, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(VkDeviceAddress), &shaderDataBuffers[frameIndex].deviceAddress);

    // Draw mesh
    vkCmdDrawIndexed(cb, indexCount, 3, 0, 0, 0);  // commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstanceID

    // Build ImGui HUD
    if(settings.show_ui)
    {
      ow_overlay::FrameData fd{
        .fps                  = fps,
        .frames_in_flight     = (int)framesInFlight,
        .fps_limited          = settings.fps_max != 0,
        .fullscreen_exclusive = settings.fullscreen,
      };
      ow_overlay::build_ui(fd);
      ow_overlay::record_draw(cb);
    }

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
      .image         = swapchainImages[imageIndex],
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
    VkPipelineStageFlags waitStages = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    // Only one graphics queue (no compute or ray tracing)
    VkSubmitInfo submitInfo{
      .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .waitSemaphoreCount   = 1,
      .pWaitSemaphores      = &presentSemaphores[frameIndex],  // wait for current frame's presentation to finish before executing
      .pWaitDstStageMask    = &waitStages,                     // wait occurs at the color attachment output stage
      .commandBufferCount   = 1,
      .pCommandBuffers      = &cb,
      .signalSemaphoreCount = 1,
      .pSignalSemaphores    = &renderSemaphores[imageIndex],  // signalled by the GPU once command buffer execution has completed
    };
    chk(vkQueueSubmit(queue, 1, &submitInfo, fences[frameIndex]));

    frameIndex = (frameIndex + 1) % framesInFlight;  // calculate frame index for next render loop iteration

    // === Present Image ===
    VkPresentInfoKHR presentInfo{
      .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores    = &renderSemaphores[imageIndex],
      .swapchainCount     = 1,
      .pSwapchains        = &swapchain,
      .pImageIndices      = &imageIndex,
    };
    chkSwapchain(vkQueuePresentKHR(queue, &presentInfo));

    if(updateFullscreen)
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
      chk(SDL_GetWindowSize(window, &windowSize.x, &windowSize.y));
      updateFullscreen = false;
      updateSwapchain  = true;
    }

    if(updateSwapchain)
    {
      chk(SDL_GetWindowSize(window, &windowSize.x, &windowSize.y));
      chk(SDL_SetWindowRelativeMouseMode(window, true));
      updateSwapchain = false;

      // Wait until the GPU has completed all outstanding operations
      chk(vkDeviceWaitIdle(device));
      chk(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(devices[settings.device], surface, &surfaceCaps));
      swapchainCI.oldSwapchain = swapchain;  // Allow the application to continue presenting any already acquired image.
      swapchainCI.imageExtent  = {
         .width  = static_cast<uint32_t>(windowSize.x),
         .height = static_cast<uint32_t>(windowSize.y),
      };
      chk(vkCreateSwapchainKHR(device, &swapchainCI, nullptr, &swapchain));

      // Destroy existing objects and create new image view
      for(auto i = 0; i < imageCount; i++)
      {
        vkDestroyImageView(device, swapchainImageViews[i], nullptr);
      }
      chk(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr));
      swapchainImages.resize(imageCount);
      chk(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data()));
      swapchainImageViews.resize(imageCount);
      for(auto i = 0; i < imageCount; i++)
      {
        VkImageViewCreateInfo viewCI{
          .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
          .image            = swapchainImages[i],
          .viewType         = VK_IMAGE_VIEW_TYPE_2D,
          .format           = imageFormat,
          .subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 }
        };
        chk(vkCreateImageView(device, &viewCI, nullptr, &swapchainImageViews[i]));
      }
      vkDestroySwapchainKHR(device, swapchainCI.oldSwapchain, nullptr);
      vmaDestroyImage(allocator, depthImage, depthImageAllocation);
      vkDestroyImageView(device, depthImageView, nullptr);

      // Create new objects
      depthImageCI.extent = {
        .width  = static_cast<uint32_t>(windowSize.x),
        .height = static_cast<uint32_t>(windowSize.y),
        .depth  = 1,
      };
      VmaAllocationCreateInfo allocCI{
        .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO,
      };
      chk(vmaCreateImage(allocator, &depthImageCI, &allocCI, &depthImage, &depthImageAllocation, nullptr));
      VkImageViewCreateInfo viewCI{
        .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image            = depthImage,
        .viewType         = VK_IMAGE_VIEW_TYPE_2D,
        .format           = depthFormat,
        .subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT, .levelCount = 1, .layerCount = 1 }
      };
      chk(vkCreateImageView(device, &viewCI, nullptr, &depthImageView));
    }
  }

  // ========================================
  // Cleaning Up
  chk(vkDeviceWaitIdle(device));  // Wait until the GPU has completed all outstanding operations
  for(auto i = 0; i < framesInFlight; i++)
  {
    vkDestroyFence(device, fences[i], nullptr);
    vkDestroySemaphore(device, presentSemaphores[i], nullptr);
    vmaDestroyBuffer(allocator, shaderDataBuffers[i].buffer, shaderDataBuffers[i].allocation);
  }
  for(auto i = 0; i < renderSemaphores.size(); i++)
  {
    vkDestroySemaphore(device, renderSemaphores[i], nullptr);
  }
  vmaDestroyImage(allocator, depthImage, depthImageAllocation);
  vkDestroyImageView(device, depthImageView, nullptr);
  for(auto i = 0; i < swapchainImageViews.size(); i++)
  {
    vkDestroyImageView(device, swapchainImageViews[i], nullptr);
  }
  vmaDestroyBuffer(allocator, vBuffer, vBufferAllocation);
  for(auto i = 0; i < textures.size(); i++)
  {
    vkDestroyImageView(device, textures[i].view, nullptr);
    vkDestroySampler(device, textures[i].sampler, nullptr);
    vmaDestroyImage(allocator, textures[i].image, textures[i].allocation);
  }
  vkDestroyDescriptorSetLayout(device, descriptorSetLayoutTex, nullptr);
  ow_overlay::shutdown(device);  // ImGui overlay
  vkDestroyDescriptorPool(device, descriptorPool, nullptr);
  vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
  vkDestroyPipeline(device, pipeline, nullptr);
  vkDestroySwapchainKHR(device, swapchain, nullptr);
  vkDestroySurfaceKHR(instance, surface, nullptr);
  vkDestroyCommandPool(device, commandPool, nullptr);  // Implicitly frees all command buffers
  vkDestroyShaderModule(device, shaderModule, nullptr);
  vmaDestroyAllocator(allocator);
  SDL_DestroyWindow(window);
  SDL_QuitSubSystem(SDL_INIT_VIDEO);
  SDL_Quit();
  vkDestroyDevice(device, nullptr);
  vkDestroyInstance(instance, nullptr);  // Should be deleted last
}


// ========================================
// Helper Functions
static inline void chk(VkResult result, const std::source_location& loc)
{
  if(result != VK_SUCCESS) [[unlikely]]
  {
    std::cerr << loc.file_name() << ":" << loc.line() << " [" << loc.function_name() << "] "
              << "Vulkan error (" << result << ")\n";
    exit(result);
  }
}

static inline void chkSwapchain(VkResult result, const std::source_location& loc)
{
  if(result < VK_SUCCESS) [[unlikely]]
  {
    if(result == VK_ERROR_OUT_OF_DATE_KHR)
    {
      updateSwapchain = true;
      return;
    }
    std::cerr << loc.file_name() << ":" << loc.line() << " [" << loc.function_name() << "] "
              << "Vulkan error (" << result << ")\n";
    exit(result);
  }
}

static inline void chk(bool result, const std::source_location& loc)
{
  if(!result) [[unlikely]]
  {
    std::cerr << loc.file_name() << ":" << loc.line() << " [" << loc.function_name() << "] "
              << "Call returned false\n";
    exit(1);
  }
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
      framesInFlight     = 1;
    }
    else if(arg == "-sensitivity")
    {
      parseFloat("sensitivity", 0.f, 100.f, s.sensitivity, i);
      camera.sensitivity = s.sensitivity;
    }
    else if(arg == "-vsync")
      s.vsync = true;
  }

  return s;
}
