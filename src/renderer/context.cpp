// #define VOLK_IMPLEMENTATION TODO: Do I put this here?

#include "context.hpp"
#include <cassert>
#include <vector>
#include <iostream>
#include "common.hpp"
#include "window.hpp"
#include <SDL3/SDL_mouse.h>
#include <entt/entity/registry.hpp>

namespace Renderer {

void initContext(entt::registry& registry)
{
  auto& ctx    = registry.ctx().get<VkContext>();
  auto& window = registry.ctx().get<WindowContext>();

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
  chk(vkCreateInstance(&instanceCI, nullptr, &ctx.instance));
  volkLoadInstance(ctx.instance);


  // ========================================
  // Device Selection
  uint32_t deviceCount{ 0 };
  chk(vkEnumeratePhysicalDevices(ctx.instance, &deviceCount, nullptr));
  std::vector<VkPhysicalDevice> devices(deviceCount);
  chk(vkEnumeratePhysicalDevices(ctx.instance, &deviceCount, devices.data()));

  // Ensure valid deviceIndex
  assert(ctx.deviceIndex < deviceCount);
  ctx.physicalDevice = devices[ctx.deviceIndex];

  // Output device name
  VkPhysicalDeviceProperties2 deviceProperties{
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
  };
  vkGetPhysicalDeviceProperties2(ctx.physicalDevice, &deviceProperties);
  std::cout << "Selected device: " << deviceProperties.properties.deviceName << "\n";


  // ========================================
  // Find a Queue Family with Graphics Support
  uint32_t queueFamilyCount{ 0 };
  vkGetPhysicalDeviceQueueFamilyProperties(ctx.physicalDevice, &queueFamilyCount, nullptr);
  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(ctx.physicalDevice, &queueFamilyCount, queueFamilies.data());
  for(size_t i = 0; i < queueFamilies.size(); i++)
  {
    if(queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
    {
      ctx.queueFamily = i;
      break;
    }
  }
  chk(SDL_Vulkan_GetPresentationSupport(ctx.instance, ctx.physicalDevice, ctx.queueFamily));


  // ========================================
  // Logical Device Setup
  const float             qfpriorities{ 1.0f };
  VkDeviceQueueCreateInfo queueCI{
    .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
    .queueFamilyIndex = ctx.queueFamily,
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
  chk(vkCreateDevice(ctx.physicalDevice, &deviceCI, nullptr, &ctx.device));
  vkGetDeviceQueue(ctx.device, ctx.queueFamily, 0, &ctx.queue);  // request queue from device


  // ========================================
  // Setting up VMA
  VmaVulkanFunctions vkFunctions{
    .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
    .vkGetDeviceProcAddr   = vkGetDeviceProcAddr,
    .vkCreateImage         = vkCreateImage,
  };
  VmaAllocatorCreateInfo allocatorCI{
    .flags            = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
    .physicalDevice   = ctx.physicalDevice,
    .device           = ctx.device,
    .pVulkanFunctions = &vkFunctions,
    .instance         = ctx.instance,
  };
  chk(vmaCreateAllocator(&allocatorCI, &ctx.allocator));


  // ========================================
  // Window and Surface
  chk(SDL_Vulkan_CreateSurface(window, ctx.instance, nullptr, &ctx.surface));
  SDL_SetWindowRelativeMouseMode(window, true);


  // ========================================
  // Command Pool
  VkCommandPoolCreateInfo commandPoolCI{
    .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
    .queueFamilyIndex = ctx.queueFamily,
  };
  chk(vkCreateCommandPool(ctx.device, &commandPoolCI, nullptr, &ctx.commandPool));
}


void destroyContext(entt::registry& registry)
{
  auto& ctx = registry.ctx().get<VkContext>();

  vkDestroySurfaceKHR(ctx.instance, ctx.surface, nullptr);
  vkDestroyCommandPool(ctx.device, ctx.commandPool, nullptr);  // Implicitly frees all command buffers
  vmaDestroyAllocator(ctx.allocator);
  vkDestroyDevice(ctx.device, nullptr);
  vkDestroyInstance(ctx.instance, nullptr);  // Should be deleted last
}

}  // namespace Renderer
