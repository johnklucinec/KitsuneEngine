#pragma once
#include <iostream>
#include <source_location>
#include <string_view>

namespace Log {
namespace Color {
inline constexpr std::string_view RESET    = "\033[0m";
inline constexpr std::string_view BOLD     = "\033[1m";
inline constexpr std::string_view CYAN     = "\033[36m";
inline constexpr std::string_view GRAY     = "\033[90m";
inline constexpr std::string_view BOLD_RED = "\033[1;31m";
}  // namespace Color

// Example:
// Log::error("ERROR", "Failed to do something", std::source_location::current());
inline void error(std::string_view type, std::string_view msg, const std::source_location loc)
{
  std::cerr << Color::BOLD_RED << "[" << type << "] " << Color::RESET << Color::BOLD << "in " << loc.function_name() << Color::RESET << "\n"
            << Color::GRAY << "  at " << loc.file_name() << ":" << loc.line() << Color::RESET << "\n"
            << "  " << Color::CYAN << "> " << Color::RESET << msg << "\n\n";
}
}  // namespace Log
