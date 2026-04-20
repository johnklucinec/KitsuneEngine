#include "factories.hpp"

#include <cassert>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <entt/entity/registry.hpp>

#include "camera.hpp"
#include "player_movement.hpp"
#include "core/settings.hpp"
#include "transform.hpp"
#include "velocity.hpp"
#include "tags.hpp"

entt::entity makePlayer(entt::registry& registry, glm::vec3 spawnPos)
{
  constexpr float BASE_SPEED = 5.5f;

  const auto* settings = registry.ctx().find<Settings>();
  assert(settings && "makePlayer: Settings must be emplaced in ctx before calling makePlayer");

  Camera camera{};
  camera.fov         = settings->fov;
  camera.sensitivity = settings->sensitivity;

  const entt::entity player = registry.create();
  registry.emplace<PlayerTag>(player);
  registry.emplace<Transform>(player, spawnPos, glm::identity<glm::quat>(), glm::vec3{ 1.f });
  registry.emplace<Camera>(player, camera);
  registry.emplace<Velocity>(player);
  registry.emplace<PlayerMovement>(player, BASE_SPEED);
  registry.emplace<CameraViewMatrix>(player);
  registry.emplace<ProjectionMatrix>(player);
  registry.emplace<DirtyCameraProjection>(player);

  return player;
}
