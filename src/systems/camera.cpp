#include "components/camera.hpp"
#include "systems/camera.hpp"
#include "components/input.hpp"
#include "components/transform.hpp"

#include <glm/gtc/quaternion.hpp>
#include <algorithm>
#include <numbers>

void sys::camera(entt::registry& reg)
{
  constexpr double K_YAW   = static_cast<double>(CAM_M_YAW) * std::numbers::pi / 180.0;
  constexpr double K_PITCH = static_cast<double>(CAM_M_PITCH) * std::numbers::pi / 180.0;

  reg.view<Camera, Transform, const Input>().each([](Camera& cam, Transform& transform, const Input& input) {
    const double sens = static_cast<double>(cam.sensitivity);

    cam.yaw_rad += static_cast<double>(input.mouse_delta.x) * K_YAW * sens;
    cam.pitch_rad += static_cast<double>(input.mouse_delta.y) * K_PITCH * sens;
    cam.pitch_rad = std::clamp(cam.pitch_rad, -CAM_PITCH_LIMIT, CAM_PITCH_LIMIT);

    const glm::quat yaw_q = glm::angleAxis(static_cast<float>(cam.yaw_rad), glm::vec3{ 0.0f, 1.0f, 0.0f });

    const glm::quat pitch_q = glm::angleAxis(static_cast<float>(cam.pitch_rad), glm::vec3{ 1.0f, 0.0f, 0.0f });

    transform.rotation = glm::normalize(yaw_q * pitch_q);
  });
}
