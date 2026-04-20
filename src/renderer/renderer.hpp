#include <entt/entity/fwd.hpp>

namespace Renderer {
void init(entt::registry& registry);
void deviceWaitIdle(entt::registry& registry);
void shutdown(entt::registry& registry);
}  // namespace Renderer
