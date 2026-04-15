#pragma once
#include "components/camera.hpp"
#include "transform.hpp"
#include <glm/glm.hpp>

namespace CamUtils {
[[nodiscard]] glm::mat4 view_matrix(const Camera& cam, const Transform& transform) noexcept;

[[nodiscard]] glm::mat4 proj_matrix(const Camera& cam, float aspect) noexcept;

[[nodiscard]] constexpr float vfov_from_hfov(float hfov_deg) noexcept;
}  // namespace CamUtils
