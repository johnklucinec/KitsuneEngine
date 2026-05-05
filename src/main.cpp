#include <entt/entity/registry.hpp>

#include "input.hpp"
#include "core/settings.hpp"
#include "core/app.hpp"
#include "core/factories.hpp"

#include "render_system.hpp"
#include "utils/settings_utils.hpp"

int main(int argc, char* argv[])
{
  entt::registry registry;

  // App config
  registry.ctx().emplace<AppState>();
  registry.ctx().emplace<Input>();
  registry.ctx().emplace<Settings>(SettingsUtils::parseArgs(argc, argv));

  // Create Player
  makePlayer(registry, { 0.0F, 0.0F, -6.0F });

  // Init + Game Loop
  System::render(registry);

  return 0;
}
