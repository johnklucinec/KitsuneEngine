## Tim Ford's Updated ECS Rules

| Rule | Note |
|------|------|
| Components have no functions |✅|
| Systems have no state | No game state, but transient state is fine if you need some scratch memory |
| Shared code lives in Utils |✅|
| Complex side effects should be deferred |✅|
| ~~Systems can't call other systems~~ | Nope, Wrong 2017 Tim - but you need to reframe what "system" is (_see below_) |

> **Key insight:** There's no difference between high-level "ticking" systems and Utils — they're all just bags of code that read or write components.
>
> I've also jettisoned the idea that an ECS world is composed of a retained list of ordered systems that all call `Update` polymorphically. That just causes pain as you add nuance to your update loop. Instead, your systems have a small public API, and you call it in whatever order makes sense for your game loop. It's just **Functions calling Functions** — easy to debug, easy to rearrange, no hidden/retained magic tickers.

---

## General EnTT System Rules

### Components
- Plain data structs — no logic, no methods (beyond maybe a constructor)
- Never store entity handles inside a component; that's a dependency, not data
- Mark `const` in views when the system only reads: `reg.view<const Foo>()`

### Systems
- Operate on components, never on specific entity IDs
- Functions take only `entt::registry&` — nothing else should be needed
- Engine-wide singletons (device, window, etc.) live in `reg.ctx()`, not as parameters

### Views
- Always use a view even for "single instance" components — never cache entity handles across frames
- A view that finds nothing iterates zero times safely; a stale handle crashes
- Multi-component filters are free: `reg.view<const Foo, const Bar>()` visits only entities with both

### Lifetime
- `emplace` in your `init` system, `destroy` in your `shutdown` system — the registry owns everything
- Prefer `reg.ctx().emplace_or_replace<T>()` for per-frame transient data (e.g. current `VkCommandBuffer`) so it's always fresh
- Never hold a raw pointer/reference to a component across frames — EnTT can reallocate storage when entities are added

### General
- One concern per component: split `struct Player { health, ammo, stamina, isGrounded }` into `Health`, `Ammo`, `Stamina`, `Grounded` so systems only pay for what they touch
