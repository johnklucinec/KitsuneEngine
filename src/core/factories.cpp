#include "factories.hpp"

#include <cassert>
#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <entt/entity/registry.hpp>
#include <random>

#include "components/camera.hpp"
#include "components/mesh.hpp"
#include "components/player_movement.hpp"
#include "components/target_movement.hpp"
#include "components/transform.hpp"
#include "components/velocity.hpp"
#include "components/tags.hpp"
#include "core/settings.hpp"

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

// what to do if invalid mesh or texture id?
entt::entity makeBot(entt::registry& registry, glm::vec3 spawnPos, uint32_t mesh, uint32_t texture)
{
  /* Make move randomly */
  std::mt19937_64                       rng(std::random_device{}());
  std::uniform_real_distribution<float> timerDist(1.0f, 5.0f);
  std::uniform_real_distribution<float> strafeDist(1.0f, 1.0f);

  const entt::entity bot = registry.create();
  registry.emplace<Transform>(bot, spawnPos, glm::identity<glm::quat>(), glm::vec3{ 1.0f });
  registry.emplace<MeshInstance>(bot, mesh);
  registry.emplace<TextureInstance>(bot, texture);
  registry.emplace<TargetMovement>(bot, timerDist(rng),             /*timer*/
                                   (rng() % 2 == 0) ? 1.0f : -1.0f, /*direction*/
                                   strafeDist(rng));                /*strafeTime*/
  registry.emplace<Velocity>(bot);

  return bot;
}
