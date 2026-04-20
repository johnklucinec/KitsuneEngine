#pragma once

#include <cstdint>
#include <vulkan/vulkan.h>
#include <source_location>
#include <iostream>
#include <cstdlib>
#include <ktx.h>

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

// clang-format off
inline const char* ktxErrStr(KTX_error_code code)
{
	switch(code)
  {
    case KTX_SUCCESS:                 	return "KTX_SUCCESS";
    case KTX_FILE_DATA_ERROR:         	return "KTX_FILE_DATA_ERROR";
    case KTX_FILE_OPEN_FAILED:        	return "KTX_FILE_OPEN_FAILED";
    case KTX_FILE_OVERFLOW:           	return "KTX_FILE_OVERFLOW";
    case KTX_FILE_READ_ERROR:         	return "KTX_FILE_READ_ERROR";
    case KTX_FILE_SEEK_ERROR:         	return "KTX_FILE_SEEK_ERROR";
    case KTX_FILE_UNEXPECTED_EOF:     	return "KTX_FILE_UNEXPECTED_EOF";
    case KTX_FILE_WRITE_ERROR:        	return "KTX_FILE_WRITE_ERROR";
    case KTX_GL_ERROR:                	return "KTX_GL_ERROR";
    case KTX_INVALID_OPERATION:       	return "KTX_INVALID_OPERATION";
    case KTX_INVALID_VALUE:           	return "KTX_INVALID_VALUE";
    case KTX_NOT_FOUND:              	 	return "KTX_NOT_FOUND";
    case KTX_OUT_OF_MEMORY:           	return "KTX_OUT_OF_MEMORY";
    case KTX_UNKNOWN_FILE_FORMAT:     	return "KTX_UNKNOWN_FILE_FORMAT";
    case KTX_UNSUPPORTED_TEXTURE_TYPE: 	return "KTX_UNSUPPORTED_TEXTURE_TYPE";
    default:                          	return "KTX_UNKNOWN_ERROR";
  }
}  // clang-format on

inline void chk(KTX_error_code result, const std::source_location& loc = std::source_location::current())
{
  if(result != KTX_SUCCESS) [[unlikely]]
  {
    std::cerr << loc.file_name() << ":" << loc.line() << " [" << loc.function_name() << "] "
              << "KTX error: " << ktxErrStr(result) << " (" << result << ")\n";
    std::exit(1);
  }
}
