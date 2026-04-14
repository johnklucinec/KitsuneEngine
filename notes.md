## EnTT System Rules

**Components**

- Components are plain data structs — no logic, no methods (beyond maybe a constructor)
- Never store entity handles inside a component; that's a dependency, not data
- Mark components `const` in views when the system only reads them — `reg.view<const Foo>()`

**Systems**

- Systems operate on components, never on specific entity IDs
- System functions take only `entt::registry&` — nothing else should be needed
- If a system needs engine-wide singletons (device, window, etc.), they live in `reg.ctx()`, not as parameters

**Views**

- Always use a view even for "single instance" components — never cache entity handles across frames
- A view that finds nothing iterates zero times safely; a stale handle crashes
- Multi-component filters are free: `reg.view<const Foo, const Bar>()` only visits entities that have both

**Lifetime**

- `emplace` in your `_init` system, `destroy` in your `_shutdown` system — the registry owns everything
- Prefer `reg.ctx().emplace_or_replace<T>()` for per-frame transient data (like the current `VkCommandBuffer`) so it's always fresh
- Never hold a raw pointer/reference to a component across frames — EnTT can reallocate storage when new entities are added

**General**

- If you're passing an entity ID into a function, that's a sign the design is fighting ECS — restructure with a component tag instead
- One concern per component; split `struct Player { health, ammo, stamina, isGrounded... }` into `Health`, `Ammo`, `Stamina`, `Grounded` so systems only pay for what they touch
