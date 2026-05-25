#include <entt/entity/registry.hpp>
#include "target_movement.hpp"
#include "transform.hpp"
#include "velocity.hpp"
#include "frame_pacer.hpp"
#include "target_behavior_system.hpp"

#include <glm/glm.hpp>

void System::targetBehavior(entt::registry& reg)
{
  const float dt   = reg.ctx().get<FrameTime>().deltaTime;
  auto        view = reg.view<Transform, Velocity, TargetMovement>();

  view.each([dt](Transform& transform, Velocity& vel, TargetMovement& target) {
    target.timer += dt;

    if(target.timer >= target.strafeTime)
    {
      target.direction = -target.direction;
      target.timer     = 0.0f;
    }

    const glm::vec2 moveInput = { target.direction, 0.0f };  // right, forward

    const float inputLengthSq = glm::dot(moveInput, moveInput);  // squared length of moveInput (faster than square root)
    if(inputLengthSq <= 0.0f)                                    // If there's no input, stop the entity
    {
      vel.linear = glm::vec3(0.0f);
      return;
    }

    const glm::vec2 moveDir       = glm::normalize(moveInput);
    const float     verticalSpeed = 0.0f;  // no vertical movement

    // Map 2D input direction onto the 3D XYZ
    vel.linear.x = moveDir.x * target.move_speed;
    vel.linear.y = verticalSpeed;
    vel.linear.z = moveDir.y * target.move_speed;

    transform.position += vel.linear * dt;
  });
}
