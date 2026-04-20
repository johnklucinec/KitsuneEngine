#pragma once
#include <cstdint>
#include <glm/glm.hpp>

// Written exclusively by the Input system.
// mouse_delta is zeroed at the start of each tick before accumulating events.
struct Input
{
  glm::dvec2 mouse_delta{ 0.0f };      // raw OS counts (SDL xrel / yrel)
  uint32_t   mouse_buttons{ 0 };       // bitmask
  uint32_t   mouse_buttons_prev{ 0 };  // held state last tick, for just_pressed/released
  uint64_t   keys{ 0 };                // current held state
  uint64_t   keys_prev{ 0 };           // held state last tick, for just_pressed/released
};

// Key bit indices (0–63)
namespace Key {
// clang-format off

// Alphabet (A–Z)
inline constexpr uint32_t A           =  0;
inline constexpr uint32_t B           =  1;
inline constexpr uint32_t C           =  2;
inline constexpr uint32_t D           =  3;
inline constexpr uint32_t E           =  4;
inline constexpr uint32_t F           =  5;
inline constexpr uint32_t G           =  6;
inline constexpr uint32_t H           =  7;
inline constexpr uint32_t I           =  8;
inline constexpr uint32_t J           =  9;
inline constexpr uint32_t K           = 10;
inline constexpr uint32_t L           = 11;
inline constexpr uint32_t M           = 12;
inline constexpr uint32_t N           = 13;
inline constexpr uint32_t O           = 14;
inline constexpr uint32_t P           = 15;
inline constexpr uint32_t Q           = 16;
inline constexpr uint32_t R           = 17;
inline constexpr uint32_t S           = 18;
inline constexpr uint32_t T           = 19;
inline constexpr uint32_t U           = 20;
inline constexpr uint32_t V           = 21;
inline constexpr uint32_t W           = 22;
inline constexpr uint32_t X           = 23;
inline constexpr uint32_t Y           = 24;
inline constexpr uint32_t Z           = 25;

// Number row
inline constexpr uint32_t Digit1      = 26;
inline constexpr uint32_t Digit2      = 27;
inline constexpr uint32_t Digit3      = 28;
inline constexpr uint32_t Digit4      = 29;
inline constexpr uint32_t Digit5      = 30;
inline constexpr uint32_t Digit6      = 31;
inline constexpr uint32_t Digit7      = 32;
inline constexpr uint32_t Digit8      = 33;
inline constexpr uint32_t Digit9      = 34;
inline constexpr uint32_t Digit0      = 35;

// Special / editing
inline constexpr uint32_t Space       = 36;
inline constexpr uint32_t Tab         = 37;
inline constexpr uint32_t Return      = 38;
inline constexpr uint32_t Backspace   = 39;
inline constexpr uint32_t Delete      = 40;
inline constexpr uint32_t Escape      = 41;
inline constexpr uint32_t Grave       = 42;  // ` / ~ — console toggle

// Arrow keys
inline constexpr uint32_t ArrowLeft   = 43;
inline constexpr uint32_t ArrowRight  = 44;
inline constexpr uint32_t ArrowUp     = 45;
inline constexpr uint32_t ArrowDown   = 46;

// Modifiers
inline constexpr uint32_t LShift      = 47;
inline constexpr uint32_t LCtrl       = 48;
inline constexpr uint32_t LAlt        = 49;
inline constexpr uint32_t RShift      = 50;
inline constexpr uint32_t RCtrl       = 51;

// Function keys
inline constexpr uint32_t F1          = 52;
inline constexpr uint32_t F2          = 53;
inline constexpr uint32_t F3          = 54;
inline constexpr uint32_t F4          = 55;
inline constexpr uint32_t F5          = 56;
inline constexpr uint32_t F6          = 57;
inline constexpr uint32_t F7          = 58;
inline constexpr uint32_t F8          = 59;
inline constexpr uint32_t F9          = 60;
inline constexpr uint32_t F10         = 61;
inline constexpr uint32_t F11         = 62;
inline constexpr uint32_t F12         = 63;

inline constexpr uint32_t Unmapped    = ~uint32_t(0);
// clang-format on
}  // namespace Key

// Mouse button bitmasks
namespace MouseBtn {
inline constexpr uint32_t Left   = 1u << 0;
inline constexpr uint32_t Middle = 1u << 1;
inline constexpr uint32_t Right  = 1u << 2;
inline constexpr uint32_t X1     = 1u << 3;
inline constexpr uint32_t X2     = 1u << 4;
}  // namespace MouseBtn

// Helpers

// Is the key currently held?
[[nodiscard]]
inline bool keyDown(const Input& in, uint32_t bit) noexcept
{
  return (in.keys >> bit) & uint64_t(1);
}

// Did the key transition from up to down this tick? (toggles, one-shots)
[[nodiscard]]
inline bool keyJustPressed(const Input& in, uint32_t bit) noexcept
{
  const uint64_t mask = uint64_t(1) << bit;
  return (in.keys & mask) && !(in.keys_prev & mask);
}

// Did the key transition from down to up this tick?
[[nodiscard]]
inline bool keyJustReleased(const Input& in, uint32_t bit) noexcept
{
  const uint64_t mask = uint64_t(1) << bit;
  return !(in.keys & mask) && (in.keys_prev & mask);
}

// Set or clear a key bit (used by the Input system only)
inline void keyWrite(Input& in, uint32_t bit, bool pressed) noexcept
{
  const uint64_t mask = uint64_t(1) << bit;
  in.keys             = pressed ? (in.keys | mask) : (in.keys & ~mask);
}

// Is the button currently held?
[[nodiscard]]
inline bool btnDown(const Input& in, uint32_t mask) noexcept
{
  return (in.mouse_buttons & mask) != 0;
}

// Did the button transition from up to down this tick? (toggles, one-shots)
[[nodiscard]]
inline bool btnJustPressed(const Input& in, uint32_t mask) noexcept
{
  return (in.mouse_buttons & mask) && !(in.mouse_buttons_prev & mask);
}

// Did the button transition from down to up this tick?
[[nodiscard]]
inline bool btnJustReleased(const Input& in, uint32_t mask) noexcept
{
  return !(in.mouse_buttons & mask) && (in.mouse_buttons_prev & mask);
}
