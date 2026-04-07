#include "systems/input.hpp"
#include "components/input.hpp"
#include "core/app.hpp"
#include <SDL3/SDL.h>

namespace {

[[nodiscard]]
constexpr uint32_t to_key_bit(SDL_Scancode sc) noexcept
{
  // clang-format off
  switch (sc)
  {
      // Alphabet
      case SDL_SCANCODE_A:         return Key::A;
      case SDL_SCANCODE_B:         return Key::B;
      case SDL_SCANCODE_C:         return Key::C;
      case SDL_SCANCODE_D:         return Key::D;
      case SDL_SCANCODE_E:         return Key::E;
      case SDL_SCANCODE_F:         return Key::F;
      case SDL_SCANCODE_G:         return Key::G;
      case SDL_SCANCODE_H:         return Key::H;
      case SDL_SCANCODE_I:         return Key::I;
      case SDL_SCANCODE_J:         return Key::J;
      case SDL_SCANCODE_K:         return Key::K;
      case SDL_SCANCODE_L:         return Key::L;
      case SDL_SCANCODE_M:         return Key::M;
      case SDL_SCANCODE_N:         return Key::N;
      case SDL_SCANCODE_O:         return Key::O;
      case SDL_SCANCODE_P:         return Key::P;
      case SDL_SCANCODE_Q:         return Key::Q;
      case SDL_SCANCODE_R:         return Key::R;
      case SDL_SCANCODE_S:         return Key::S;
      case SDL_SCANCODE_T:         return Key::T;
      case SDL_SCANCODE_U:         return Key::U;
      case SDL_SCANCODE_V:         return Key::V;
      case SDL_SCANCODE_W:         return Key::W;
      case SDL_SCANCODE_X:         return Key::X;
      case SDL_SCANCODE_Y:         return Key::Y;
      case SDL_SCANCODE_Z:         return Key::Z;

      // Number row
      case SDL_SCANCODE_1:         return Key::Digit1;
      case SDL_SCANCODE_2:         return Key::Digit2;
      case SDL_SCANCODE_3:         return Key::Digit3;
      case SDL_SCANCODE_4:         return Key::Digit4;
      case SDL_SCANCODE_5:         return Key::Digit5;
      case SDL_SCANCODE_6:         return Key::Digit6;
      case SDL_SCANCODE_7:         return Key::Digit7;
      case SDL_SCANCODE_8:         return Key::Digit8;
      case SDL_SCANCODE_9:         return Key::Digit9;
      case SDL_SCANCODE_0:         return Key::Digit0;

      // Special / editing
      case SDL_SCANCODE_SPACE:     return Key::Space;
      case SDL_SCANCODE_TAB:       return Key::Tab;
      case SDL_SCANCODE_RETURN:    return Key::Return;
      case SDL_SCANCODE_BACKSPACE: return Key::Backspace;
      case SDL_SCANCODE_DELETE:    return Key::Delete;
      case SDL_SCANCODE_ESCAPE:    return Key::Escape;
      case SDL_SCANCODE_GRAVE:     return Key::Grave;

      // Arrow keys
      case SDL_SCANCODE_LEFT:      return Key::ArrowLeft;
      case SDL_SCANCODE_RIGHT:     return Key::ArrowRight;
      case SDL_SCANCODE_UP:        return Key::ArrowUp;
      case SDL_SCANCODE_DOWN:      return Key::ArrowDown;

      // Modifiers
      case SDL_SCANCODE_LSHIFT:    return Key::LShift;
      case SDL_SCANCODE_LCTRL:     return Key::LCtrl;
      case SDL_SCANCODE_LALT:      return Key::LAlt;
      case SDL_SCANCODE_RSHIFT:    return Key::RShift;
      case SDL_SCANCODE_RCTRL:     return Key::RCtrl;

      // Function keys
      case SDL_SCANCODE_F1:        return Key::F1;
      case SDL_SCANCODE_F2:        return Key::F2;
      case SDL_SCANCODE_F3:        return Key::F3;
      case SDL_SCANCODE_F4:        return Key::F4;
      case SDL_SCANCODE_F5:        return Key::F5;
      case SDL_SCANCODE_F6:        return Key::F6;
      case SDL_SCANCODE_F7:        return Key::F7;
      case SDL_SCANCODE_F8:        return Key::F8;
      case SDL_SCANCODE_F9:        return Key::F9;
      case SDL_SCANCODE_F10:       return Key::F10;
      case SDL_SCANCODE_F11:       return Key::F11;
      case SDL_SCANCODE_F12:       return Key::F12;

      default:                     return Key::Unmapped;
  }
  // clang-format on
}

}  // anonymous namespace

namespace sys {

void input(entt::registry& reg)
{
  auto& app  = reg.ctx().get<AppState>();
  auto  view = reg.view<Input>();

  //Snapshot + reset per-frame transient state
  // keys_prev is snapshotted BEFORE new events are processed so that
  // key_just_pressed / key_just_released reflect transitions this tick only.
  view.each([](Input& in) {
    in.keys_prev   = in.keys;
    in.mouse_delta = glm::vec2{ 0.0f };
  });

  // Drain the OS event queue
  SDL_Event ev;

  while(SDL_PollEvent(&ev))
  {
    switch(ev.type)
    {

      case SDL_EVENT_QUIT:
        app.running = false;
        break;

      case SDL_EVENT_WINDOW_RESIZED:
        app.resize_swapchain = true;
        break;

      case SDL_EVENT_MOUSE_MOTION:
        view.each([&](Input& in) {
          in.mouse_delta.x += static_cast<float>(ev.motion.xrel);
          in.mouse_delta.y += static_cast<float>(ev.motion.yrel);
        });
        break;

      case SDL_EVENT_KEY_DOWN:
      case SDL_EVENT_KEY_UP: {
        const uint32_t bit     = to_key_bit(ev.key.scancode);
        const bool     pressed = (ev.type == SDL_EVENT_KEY_DOWN);
        if(bit != Key::Unmapped)
        {
          view.each([&](Input& in) { key_write(in, bit, pressed); });
        }
        break;
      }

      case SDL_EVENT_MOUSE_BUTTON_DOWN:
      case SDL_EVENT_MOUSE_BUTTON_UP: {
        const uint32_t mask    = 1u << (ev.button.button - 1u);
        const bool     pressed = (ev.type == SDL_EVENT_MOUSE_BUTTON_DOWN);
        view.each([&](Input& in) {
          if(pressed)
            in.mouse_buttons |= mask;
          else
            in.mouse_buttons &= ~mask;
        });
        break;
      }

      default:
        break;
    }
  }
}

}  // namespace sys
