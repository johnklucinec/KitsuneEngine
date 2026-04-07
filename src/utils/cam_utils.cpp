#include "cam_utils.hpp"
#include <glm/gtc/matrix_transform.hpp>

glm::mat4 CamUtils::view_matrix(const Camera& cam, const Transform& transform) noexcept
{
  const glm::mat4 rot_yaw = glm::rotate(glm::mat4(1.0f),                  //
                                        static_cast<float>(cam.yaw_rad),  //
                                        glm::vec3(0.0f, 1.0f, 0.0f));     //

  const glm::mat4 rot_pitch = glm::rotate(glm::mat4(1.0f),                     //
                                          -static_cast<float>(cam.pitch_rad),  //
                                          glm::vec3(1.0f, 0.0f, 0.0f));        //

  return rot_pitch * rot_yaw * glm::translate(glm::mat4(1.0f), transform.position);
}

glm::mat4 CamUtils::proj_matrix(const Camera& cam, float aspect) noexcept
{
  return glm::perspective(vfov_from_hfov(cam.fov), aspect, cam.z_near, cam.z_far);
}

// Convert horizontal FOV (degrees) → vertical FOV (radians) at a given aspect.
// Defaults to CAM_REF_ASPECT (16:9) so VFOV — and vertical sensitivity — stay
// constant across all resolutions; horizontal FOV expands on wider screens ("Hor+").
// Pass result to glm::perspective(). Call once when the FOV setting changes.
[[nodiscard]]
constexpr float CamUtils::vfov_from_hfov(float hfov_deg, float aspect) noexcept
{
  constexpr float DEG_TO_RAD = std::numbers::pi_v<float> / 180.0f;
  const float     h_rad      = hfov_deg * DEG_TO_RAD;
  return 2.0f * std::atan(std::tan(h_rad * 0.5f) / aspect);
}
