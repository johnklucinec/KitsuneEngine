/* Copyright (c) 2025-2026, Johnny Klucinec
 * SPDX-License-Identifier: MIT */

#include <SDL3/SDL_video.h>
#include <cassert>
#include <vulkan/vulkan_core.h>
#define VOLK_IMPLEMENTATION
#include <vulkan/vulkan.h>
#include <volk/volk.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vector>
#include <array>
#include <string>
#include <iostream>
#include <fstream>
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

// Globals
VkDevice       device{ VK_NULL_HANDLE };
VkImage        depthImage;
VkImageView    depthImageView;
VkInstance     instance{ VK_NULL_HANDLE };
VkQueue        queue{ VK_NULL_HANDLE };
VkSurfaceKHR   surface{ VK_NULL_HANDLE };
VkSwapchainKHR swapchain{ VK_NULL_HANDLE };
bool           updateSwapchain{ false };
VkBuffer       vBuffer{ VK_NULL_HANDLE };

std::vector<VkImage>     swapchainImages;
std::vector<VkImageView> swapchainImageViews;

VmaAllocator  allocator{ VK_NULL_HANDLE };
VmaAllocation depthImageAllocation;
VmaAllocation vBufferAllocation{ VK_NULL_HANDLE };

glm::ivec2 windowSize{};

struct Vertex
{
  glm::vec3 pos;
  glm::vec3 normal;
  glm::vec2 uv;
};

// Helper Function Prototypes
static inline void chk(VkResult result, const std::source_location& loc = std::source_location::current());
static inline void chkSwapchain(VkResult result, const std::source_location& loc = std::source_location::current());
static inline void chk(bool result, const std::source_location& loc = std::source_location::current());


int main(int argc, char* argv[])
{
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

  // ========================================
  // Device Selection
  uint32_t deviceCount{ 0 };
  chk(vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr));
  std::vector<VkPhysicalDevice> devices(deviceCount);
  chk(vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data()));

  // Optional: select device from command line
  uint32_t deviceIndex{ 0 };
  if(argc > 1)
  {
    deviceIndex = std::stoi(argv[1]);
    assert(deviceIndex < deviceCount);
  }

  // Output device name
  VkPhysicalDeviceProperties2 deviceProperties{
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
  };
  vkGetPhysicalDeviceProperties2(devices[deviceIndex], &deviceProperties);
  std::cout << "Selected device: " << deviceProperties.properties.deviceName << "\n";

  // ========================================
  // Find a Queue Family with Graphics Support
  uint32_t queueFamilyCount{ 0 };
  vkGetPhysicalDeviceQueueFamilyProperties(devices[deviceIndex], &queueFamilyCount, nullptr);
  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(devices[deviceIndex], &queueFamilyCount, queueFamilies.data());
  uint32_t queueFamily{ 0 };
  for(size_t i = 0; i < queueFamilies.size(); i++)
  {
    if(queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
    {
      queueFamily = i;
      break;
    }
  }
  chk(SDL_Vulkan_GetPresentationSupport(instance, devices[deviceIndex], queueFamily));

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
  chk(vkCreateDevice(devices[deviceIndex], &deviceCI, nullptr, &device));
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
    .physicalDevice   = devices[deviceIndex],
    .device           = device,
    .pVulkanFunctions = &vkFunctions,
    .instance         = instance,
  };
  chk(vmaCreateAllocator(&allocatorCI, &allocator));

  // ========================================
  // Window and Surface
  SDL_Window* window = SDL_CreateWindow("How to Vulkan", 1920u, 1080u, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
  assert(window);
  chk(SDL_Vulkan_CreateSurface(window, instance, nullptr, &surface));  // request vulkan surface
  chk(SDL_GetWindowSize(window, &windowSize.x, &windowSize.y));
  VkSurfaceCapabilitiesKHR surfaceCaps{};
  chk(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(devices[deviceIndex], surface, &surfaceCaps));  // request surface properties

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
    .presentMode      = VK_PRESENT_MODE_FIFO_KHR,
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
    vkGetPhysicalDeviceFormatProperties2(devices[deviceIndex], format, &formatProperties);
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

  // COMING SOON
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
