#include "components/camera.hpp"
#include "components/input.hpp"
#include "components/player_movement.hpp"
#include "components/transform.hpp"
#include "components/velocity.hpp"
#include "components/frame_pacer.hpp"
#include "systems/player_movement.hpp"

#include <glm/glm.hpp>
#include <cmath>

inline constexpr float PC_SPRINT_MULT    = 1.5f;
inline constexpr float PC_BACKWARD_MULT  = 0.90f;
inline constexpr float PC_BACK_DIAG_MULT = (0.933333f + 1.0f) * 0.5f;
inline constexpr float PC_AXIS_EPS       = 1e-3f;

void sys::player_movement(entt::registry& reg)
{
  const auto& input = reg.ctx().get<Input>();
  const auto& dt    = reg.ctx().get<FrameTime>().deltaTime;

  reg.view<Transform, Velocity, const PlayerMovement, const Camera>().each([&input, dt](Transform& transform, Velocity& vel, const PlayerMovement& pc, const Camera& cam) {
    const float side   = (key_down(input, Key::A) ? 1.0f : 0.0f) - (key_down(input, Key::D) ? 1.0f : 0.0f);
    const float fwd    = (key_down(input, Key::W) ? 1.0f : 0.0f) - (key_down(input, Key::S) ? 1.0f : 0.0f);
    const bool  sprint = key_down(input, Key::LShift);

    glm::vec2   wish{ side, fwd };
    const float len_sq = wish.x * wish.x + wish.y * wish.y;

    if(len_sq <= 0.0f)
    {
      vel.linear = glm::vec3{ 0.0f };
      return;
    }

    wish = glm::normalize(wish);

    // Directional speed penalties (Overwatch-style)
    const bool has_fwd  = fwd > PC_AXIS_EPS;
    const bool has_back = fwd < -PC_AXIS_EPS;
    const bool has_side = std::abs(side) > PC_AXIS_EPS;

    float dir_mult = 1.0f;
    if(has_back && !has_side)
      dir_mult = PC_BACKWARD_MULT;
    else if(has_back && has_side)
      dir_mult = PC_BACK_DIAG_MULT;

    float speed = pc.move_speed * dir_mult;
    if(sprint && has_fwd)
      speed *= PC_SPRINT_MULT;

    // Rotate local wish to world XZ via camera yaw
    const float cy = std::cos(static_cast<float>(cam.yaw_rad));
    const float sy = std::sin(static_cast<float>(cam.yaw_rad));

    vel.linear.x = (wish.x * cy - wish.y * sy) * speed;
    vel.linear.z = (wish.x * sy + wish.y * cy) * speed;

    // Integrate into world position
    // TODO: This should eventually be its own system: view<Transform, Velocity>.each → position += vel * dt
    transform.position += vel.linear * dt;
  });
}
