#include "components/camera.hpp"
#include "components/input.hpp"
#include "components/player_movement.hpp"
#include "components/transform.hpp"
#include "components/velocity.hpp"
#include "systems/player_movement.hpp"
#include "tags.hpp"

#include <glm/glm.hpp>
#include <cmath>

void sys::player_movement(entt::registry& reg, float dt)
{
  reg.view<Transform, Velocity, const PlayerMovement, const Camera, const Input>().each(
      [&reg, dt](entt::entity e, Transform& transform, Velocity& vel, const PlayerMovement& pc, const Camera& cam, const Input& input) {
        // Build local-space wish vector (right, forward)
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

        wish *= 1.0f / std::sqrt(len_sq);  // normalize

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

        // Rotate local wish → world XZ via camera yaw
        const float cy = std::cos(static_cast<float>(cam.yaw_rad));
        const float sy = std::sin(static_cast<float>(cam.yaw_rad));

        vel.linear.x = (wish.x * cy - wish.y * sy) * speed;
        vel.linear.y = 0.0f;  // no vertical movement here
        vel.linear.z = (wish.x * sy + wish.y * cy) * speed;

        // Integrate into world position
        transform.position += vel.linear * dt;

        reg.emplace_or_replace<DirtyCameraTransform>(e); // Recalculate ProjectionMatrix
      });
}
