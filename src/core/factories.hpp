#pragma once
#include <entt/entity/fwd.hpp>
#include <glm/vec3.hpp>

entt::entity makePlayer(entt::registry& registry, glm::vec3 spawnPos = { 0.0f, 0.0f, 0.0f });
entt::entity makeBot(entt::registry& registry, glm::vec3 spawnPos = { 0.0f, 0.0f, 0.0f }, uint32_t mesh = 0u, uint32_t texture = 0u);
