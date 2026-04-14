#include "systems/camera.hpp"
#include "components/camera.hpp"
#include "components/input.hpp"
#include "components/transform.hpp"
#include "tags.hpp"
#include "utils/cam_utils.hpp"

#include <glm/gtc/quaternion.hpp>
#include <algorithm>
#include <numbers>
#include <entt/entt.hpp>

void sys::camera(entt::registry& reg, uint32_t vp_width, uint32_t vp_height)
{
  // Pass 1: rotate camera from mouse input, mark entity dirty for Pass 2
  constexpr double K_YAW   = static_cast<double>(CAM_M_YAW) * std::numbers::pi / 180.0;
  constexpr double K_PITCH = static_cast<double>(CAM_M_PITCH) * std::numbers::pi / 180.0;

  reg.view<Camera, Transform, const Input>().each([&reg](entt::entity e, Camera& cam, Transform& t, const Input& in) {
    // Skip when there's no mouse movement
    if(in.mouse_delta.x == 0.0f && in.mouse_delta.y == 0.0f)
      return;

    const double sens = static_cast<double>(cam.sensitivity);

    cam.yaw_rad += in.mouse_delta.x * K_YAW * sens;
    cam.pitch_rad += in.mouse_delta.y * K_PITCH * sens;
    cam.pitch_rad = std::clamp(cam.pitch_rad, -CAM_PITCH_LIMIT, CAM_PITCH_LIMIT);

    const glm::quat yaw_q   = glm::angleAxis(static_cast<float>(cam.yaw_rad), glm::vec3{ 0.0f, 1.0f, 0.0f });
    const glm::quat pitch_q = glm::angleAxis(static_cast<float>(cam.pitch_rad), glm::vec3{ 1.0f, 0.0f, 0.0f });

    t.rotation = glm::normalize(yaw_q * pitch_q);

    reg.emplace_or_replace<DirtyCameraTransform>(e);  // tag for Pass 2 (Recalculate ProjectionMatrix)
  });

  // Pass 2: rebuild view matrix for dirty cameras, then clear the tag
  reg.view<DirtyCameraTransform, Camera, Transform, WorldMatrix>().each(
      [](entt::entity, Camera& cam, Transform& t, WorldMatrix& wm) { wm.matrix = CamUtils::view_matrix(cam, t); });
  reg.clear<DirtyCameraTransform>();

  // Pass 3: rebuild projection for cameras flagged dirty (resize, FOV change, etc.)
  if(vp_height == 0)
    return;

  const float aspect = static_cast<float>(vp_width) / static_cast<float>(vp_height);

  reg.view<DirtyCameraProjection, Camera, ProjectionMatrix>().each(
      [aspect](entt::entity, Camera& cam, ProjectionMatrix& pm) { pm.matrix = CamUtils::proj_matrix(cam, aspect); });
  reg.clear<DirtyCameraProjection>();
}
