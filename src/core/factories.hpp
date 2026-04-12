#pragma once

#include <entt/entity/fwd.hpp>
#include <glm/vec3.hpp>

entt::entity makePlayer(entt::registry& registry, glm::vec3 spawnPos = { 0.0f, 0.0f, 0.0f });
