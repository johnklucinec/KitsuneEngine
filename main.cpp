/* Copyright (c) 2025-2026, Johnny Klucinec
 * SPDX-License-Identifier: MIT
 */

#include <vulkan/vulkan_core.h>
#define VOLK_IMPLEMENTATION
#define VK_USE_PLATFORM_WIN32_KHR
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
VkDevice   device{ VK_NULL_HANDLE };
VkInstance instance{ VK_NULL_HANDLE };
VkQueue    queue{ VK_NULL_HANDLE };
bool       updateSwapchain{ false };

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

  // optional: select device from command line
  uint32_t deviceIndex{ 0 };
  if(argc > 1)
  {
    deviceIndex = std::stoi(argv[1]);
    assert(deviceIndex < deviceCount);
  }

  // output device name
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
  vkGetDeviceQueue(device, queueFamily, 0, &queue); // request queue from device
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
