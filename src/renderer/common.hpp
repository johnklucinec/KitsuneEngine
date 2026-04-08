#pragma once

#include <cstdint>
#include <vulkan/vulkan.h>
#include <source_location>
#include <iostream>
#include <cstdlib>

inline constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

/// Checks a VkResult and aborts with file/line/function diagnostic on failure.
inline void chk(VkResult result, const std::source_location& loc = std::source_location::current())
{
  if(result != VK_SUCCESS) [[unlikely]]
  {
    std::cerr << loc.file_name() << ":" << loc.line() << " [" << loc.function_name() << "] "
              << "Vulkan error (" << result << ")\n";
    std::exit(result);
  }
}

/// Checks a bool return (SDL3, assertions) and aborts on false.
inline void chk(bool result, const std::source_location& loc = std::source_location::current())
{
  if(!result) [[unlikely]]
  {
    std::cerr << loc.file_name() << ":" << loc.line() << " [" << loc.function_name() << "] "
              << "Call returned false\n";
    std::exit(1);
  }
}

/// Checks a swapchain VkResult.
/// Returns true  → swapchain is out of date, caller should set updateSwapchain = true.
/// Returns false → result was VK_SUCCESS or VK_SUBOPTIMAL_KHR (non-fatal).
/// Aborts        → any other negative VkResult.
[[nodiscard]] inline bool chkSwapchain(VkResult result, const std::source_location& loc = std::source_location::current())
{
  if(result >= VK_SUCCESS)  // VK_SUCCESS (0) and VK_SUBOPTIMAL_KHR (1) both pass
    return false;

  if(result == VK_ERROR_OUT_OF_DATE_KHR)
    return true;  // non-fatal; caller rebuilds the swapchain

  std::cerr << loc.file_name() << ":" << loc.line() << " [" << loc.function_name() << "] "
            << "Vulkan error (" << result << ")\n";
  std::exit(result);
}
