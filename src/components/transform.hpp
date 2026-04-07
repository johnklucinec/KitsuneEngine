#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

// World-space position/orientation/scale.
struct Transform
{
    glm::vec3 position { 0.0f, 0.0f, 0.0f };
    glm::quat rotation { 1.0f, 0.0f, 0.0f, 0.0f };  // identity (w, x, y, z)
    glm::vec3 scale    { 1.0f, 1.0f, 1.0f };
};
