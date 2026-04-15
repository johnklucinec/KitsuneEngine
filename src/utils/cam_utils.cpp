#include "cam_utils.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <numbers>

constexpr float CAM_REF_ASPECT = 16.0f / 9.0f;

glm::mat4 CamUtils::view_matrix(const Camera& cam, const Transform& transform) noexcept
{
  const glm::mat4 rot_yaw   = glm::rotate(glm::mat4(1.0f), static_cast<float>(cam.yaw_rad), glm::vec3(0, 1, 0));
  const glm::mat4 rot_pitch = glm::rotate(glm::mat4(1.0f), -static_cast<float>(cam.pitch_rad), glm::vec3(1, 0, 0));

  return rot_pitch * rot_yaw * glm::translate(glm::mat4(1.0f), transform.position);
}

glm::mat4 CamUtils::proj_matrix(const Camera& cam, float aspect) noexcept
{
  return glm::perspective(vfov_from_hfov(cam.fov), aspect, cam.z_near, cam.z_far);
}

// Takes a horizontal FOV in degrees and returns a vertical FOV in radians, which is what glm::perspective() wants.
// We lock to a 16:9 reference aspect so the vertical FOV (and vertical sensitivity) never changes between resolutions.
// Wider screens just get more horizontal view (Hor+ scaling).
[[nodiscard]] constexpr float CamUtils::vfov_from_hfov(float hfov_deg) noexcept
{
  constexpr float DEG_TO_RAD = std::numbers::pi_v<float> / 180.0f;
  const float     h_rad      = hfov_deg * DEG_TO_RAD;
  return 2.0f * std::atan(std::tan(h_rad * 0.5f) / CAM_REF_ASPECT);
}
