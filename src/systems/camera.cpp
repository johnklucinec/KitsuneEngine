#include "systems/camera.hpp"
#include "components/camera.hpp"
#include "components/input.hpp"
#include "components/transform.hpp"
#include "tags.hpp"
#include "utils/cam_utils.hpp"
#include "window.hpp"

#include <glm/gtc/quaternion.hpp>
#include <algorithm>
#include <numbers>
#include <entt/entt.hpp>

inline constexpr float CAM_M_YAW   = 0.006666f;
inline constexpr float CAM_M_PITCH = 0.006666f;

// Max pitch before gimbal / view flip (~89.8°)
inline constexpr double CAM_PITCH_LIMIT = std::numbers::pi * 0.499;

void sys::camera(entt::registry& reg)
{

  constexpr double K_YAW   = static_cast<double>(CAM_M_YAW) * std::numbers::pi / 180.0;
  constexpr double K_PITCH = static_cast<double>(CAM_M_PITCH) * std::numbers::pi / 180.0;

  const auto& in = reg.ctx().get<Input>();
  const auto& wi = reg.ctx().get<WindowContext>();

  // Rotate camera from mouse input
  reg.view<Camera, Transform, CameraViewMatrix>().each([&](Camera& cam, Transform& t, CameraViewMatrix& vm) {
    cam.yaw_rad += in.mouse_delta.x * K_YAW * static_cast<double>(cam.sensitivity);
    cam.pitch_rad += in.mouse_delta.y * K_PITCH * static_cast<double>(cam.sensitivity);
    cam.pitch_rad = std::clamp(cam.pitch_rad, -CAM_PITCH_LIMIT, CAM_PITCH_LIMIT);

    const glm::quat yaw_q   = glm::angleAxis(static_cast<float>(cam.yaw_rad), glm::vec3{ 0.0f, 1.0f, 0.0f });
    const glm::quat pitch_q = glm::angleAxis(static_cast<float>(cam.pitch_rad), glm::vec3{ 1.0f, 0.0f, 0.0f });

    t.rotation = glm::normalize(yaw_q * pitch_q);
    vm.matrix  = CamUtils::view_matrix(cam, t);
  });


  // Rebuild projection for cameras flagged dirty (resize, FOV change, etc.)
  if(wi.height == 0)
    return;

  const float aspect = static_cast<float>(wi.width) / static_cast<float>(wi.height);

  reg.view<DirtyCameraProjection, Camera, ProjectionMatrix>().each(
      [aspect](entt::entity, Camera& cam, ProjectionMatrix& pm) { pm.matrix = CamUtils::proj_matrix(cam, aspect); });
  reg.clear<DirtyCameraProjection>();
}
