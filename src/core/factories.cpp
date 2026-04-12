#include "factories.hpp"

#include <cassert>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <entt/entt.hpp>

#include "components/camera.hpp"
#include "components/input.hpp"
#include "components/player_movement.hpp"
#include "core/settings.hpp"
#include "components/transform.hpp"
#include "components/velocity.hpp"
#include "components/tags.hpp"

entt::entity makePlayer(entt::registry& registry, glm::vec3 spawnPos)
{
    const auto* settings = registry.ctx().find<Settings>();
    assert(settings && "makePlayer: Settings must be emplaced in ctx before calling makePlayer");

    Camera camera{};
    camera.fov         = settings->fov;
    camera.sensitivity = settings->sensitivity;

    const entt::entity player = registry.create();
    registry.emplace<PlayerTag>(player);
    registry.emplace<Input>(player);
    registry.emplace<Transform>(player, spawnPos, glm::quat{}, glm::vec3{ 1.f });
    registry.emplace<Camera>(player, camera);
    registry.emplace<Velocity>(player);
    registry.emplace<PlayerMovement>(player);
    registry.emplace<WorldMatrix>(player);
    registry.emplace<ProjectionMatrix>(player);

    return player;
}
