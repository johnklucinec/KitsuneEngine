#include <entt/entt.hpp>

#include "core/settings.hpp"
#include "core/app.hpp"
#include "core/factories.hpp"

#include "temp_app.hpp"

int main(int argc, char* argv[])
{
  entt::registry registry;

  // App config
  registry.ctx().emplace<AppState>();
  const auto& settings = registry.ctx().emplace<Settings>(parseArgs(argc, argv));

  // Create Player
  makePlayer(registry, { 0.0F, 0.0F, -6.0F });

  // Init + Game Loop
  return run(registry);

  return 0;
}
