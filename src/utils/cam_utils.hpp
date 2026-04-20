#pragma once
#include "camera.hpp"
#include "transform.hpp"
#include <glm/glm.hpp>

namespace CamUtils {
[[nodiscard]] glm::mat4 viewMatrix(const Camera& cam, const Transform& transform) noexcept;

[[nodiscard]] glm::mat4 projMatrix(const Camera& cam, float aspect) noexcept;

[[nodiscard]] constexpr float vfovFromHfov(float hfov_deg) noexcept;
}  // namespace CamUtils
