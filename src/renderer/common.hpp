#pragma once

#include "utils/log_utils.hpp"
#include <vulkan/vulkan.h>
#include <ktx.h>
#include <cstdlib>
#include <string>

inline constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;
constexpr uint32_t        MAX_TEXTURES         = 256;

namespace {
inline const char* vkErrStr(VkResult result)
{
  switch(result)
  {
    case VK_SUCCESS:
      return "VK_SUCCESS";
    case VK_NOT_READY:
      return "VK_NOT_READY";
    case VK_TIMEOUT:
      return "VK_TIMEOUT";
    case VK_EVENT_SET:
      return "VK_EVENT_SET";
    case VK_EVENT_RESET:
      return "VK_EVENT_RESET";
    case VK_INCOMPLETE:
      return "VK_INCOMPLETE";
    case VK_ERROR_OUT_OF_HOST_MEMORY:
      return "VK_ERROR_OUT_OF_HOST_MEMORY";
    case VK_ERROR_OUT_OF_DEVICE_MEMORY:
      return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
    case VK_ERROR_INITIALIZATION_FAILED:
      return "VK_ERROR_INITIALIZATION_FAILED";
    case VK_ERROR_DEVICE_LOST:
      return "VK_ERROR_DEVICE_LOST";
    case VK_ERROR_MEMORY_MAP_FAILED:
      return "VK_ERROR_MEMORY_MAP_FAILED";
    case VK_ERROR_LAYER_NOT_PRESENT:
      return "VK_ERROR_LAYER_NOT_PRESENT";
    case VK_ERROR_EXTENSION_NOT_PRESENT:
      return "VK_ERROR_EXTENSION_NOT_PRESENT";
    case VK_ERROR_FEATURE_NOT_PRESENT:
      return "VK_ERROR_FEATURE_NOT_PRESENT";
    case VK_ERROR_INCOMPATIBLE_DRIVER:
      return "VK_ERROR_INCOMPATIBLE_DRIVER";
    case VK_ERROR_TOO_MANY_OBJECTS:
      return "VK_ERROR_TOO_MANY_OBJECTS";
    case VK_ERROR_FORMAT_NOT_SUPPORTED:
      return "VK_ERROR_FORMAT_NOT_SUPPORTED";
    case VK_ERROR_FRAGMENTED_POOL:
      return "VK_ERROR_FRAGMENTED_POOL";
    case VK_ERROR_UNKNOWN:
      return "VK_ERROR_UNKNOWN";
    case VK_ERROR_OUT_OF_DATE_KHR:
      return "VK_ERROR_OUT_OF_DATE_KHR";
    case VK_SUBOPTIMAL_KHR:
      return "VK_SUBOPTIMAL_KHR";
    default:
      return "VK_UNKNOWN_ERROR";
  }
}

// clang-format off
inline const char* ktxErrStr(KTX_error_code code) {
    switch(code) {
        case KTX_SUCCESS:                return "KTX_SUCCESS";
        case KTX_FILE_DATA_ERROR:        return "KTX_FILE_DATA_ERROR";
        case KTX_FILE_OPEN_FAILED:       return "KTX_FILE_OPEN_FAILED";
        case KTX_FILE_OVERFLOW:          return "KTX_FILE_OVERFLOW";
        case KTX_FILE_READ_ERROR:        return "KTX_FILE_READ_ERROR";
        case KTX_FILE_SEEK_ERROR:        return "KTX_FILE_SEEK_ERROR";
        case KTX_FILE_UNEXPECTED_EOF:    return "KTX_FILE_UNEXPECTED_EOF";
        case KTX_FILE_WRITE_ERROR:       return "KTX_FILE_WRITE_ERROR";
        case KTX_INVALID_OPERATION:      return "KTX_INVALID_OPERATION";
        case KTX_INVALID_VALUE:          return "KTX_INVALID_VALUE";
        case KTX_NOT_FOUND:              return "KTX_NOT_FOUND";
        case KTX_OUT_OF_MEMORY:          return "KTX_OUT_OF_MEMORY";
        case KTX_UNKNOWN_FILE_FORMAT:    return "KTX_UNKNOWN_FILE_FORMAT";
        case KTX_UNSUPPORTED_TEXTURE_TYPE: return "KTX_UNSUPPORTED_TEXTURE_TYPE";
        default:                         return "KTX_UNKNOWN_ERROR";
    }
}  // clang-format on
}  // namespace


/**
 * @brief Checks a VkResult. Aborts on any error.
 */
inline void chk(VkResult result, const std::source_location loc = std::source_location::current())
{
  if(result != VK_SUCCESS) [[unlikely]]
  {
    std::string msg = std::string(vkErrStr(result)) + " (" + std::to_string(result) + ")";
    Log::error("VkResult Error", msg, loc);
    std::exit(static_cast<int>(result));
  }
}

/**
 * @brief Checks a boolean. Aborts if false. Useful for SDL3 or general assertions.
 */
inline void chk(bool result, const std::source_location loc = std::source_location::current())
{
  if(!result) [[unlikely]]
  {
    Log::error("Check Error", "Condition evaluated to false / API call failed", loc);
    std::exit(1);
  }
}

/**
 * @brief Specialized check for Swapchain acquisition/presentation.
 * @return true if the swapchain needs to be rebuilt, false otherwise.
 */
[[nodiscard]] inline bool chkSwapchain(VkResult result, const std::source_location loc = std::source_location::current())
{
  if(result >= VK_SUCCESS)
    return false;
  if(result == VK_ERROR_OUT_OF_DATE_KHR)
    return true;

  std::string msg = std::string(vkErrStr(result)) + " (" + std::to_string(result) + ")";
  Log::error("Swapchain Error", msg, loc);
  std::exit(static_cast<int>(result));
}

/**
 * @brief Checks a KTX error code. Aborts on failure.
 */
inline void chk(KTX_error_code result, const std::source_location loc = std::source_location::current())
{
  if(result != KTX_SUCCESS) [[unlikely]]
  {
    std::string msg = std::string(ktxErrStr(result)) + " (" + std::to_string(result) + ")";
    Log::error("KTX Error", msg, loc);
    std::exit(1);
  }
}
