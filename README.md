# Kitsune Engine - A Vulkan Game Engine

This is a _very_ early build.

## Info
- ECS (using EnTT)
- Forward (not Forward+, since less than 5 dynamic lights)
- Burley + GGX
- Overwatch FOV (103deg)
- Overwatch Sensitivity [arg: set sensitivity with -sense <value>]
- Overwatch Movement
- Simple overlay (fps, crosshair, current settings)
- FPS Limiter (Win32 high-res waitable timer + spin-wait, ~1ms precision)

## Immediate Plans

- Convert obj --> glTF
- Load multiple models using instancing (dynamic through ECS, not by modifying engine)
- Implement raycast, lifespan, scenario, scoring, and target systems (and ones I forgot)

## Semi-Immediate Plans

- Fullscreen Exclusive vs Fullscreen Borderless
- Smart check for all display modes
- Disable unused SLD3 features
- Lock 16:9 aspect ratio?
- Explain Info in more detail
- Betting settings / controls systems. Settings.ini file AND launch options
- In engine profiling


## Long Term Plans
* Turn into a simple FPS Aim Trainer (Like Kovaaks)
* Make FPS Limiter Linux compatable + VK_EXT_present_timing (if that makes sense even)
* Make CMAKE better for release build
* Convert to VK_EXT_descriptor_heap (Two backends in an RHI?)
* Convert to Forward+. Mainly when the engine gets more advanced
---

## Settings! (Launch Options)

| Setting          | Default | Description                       |
| ---------------- | ------- | --------------------------------- |
| device           | 0       | GPU Device Index                  |
| fov              | 103.0f  | Horizontal FOV (`30‑160`)         |
| fps_max          | 120.0f  | Max FPS (`1‑9990`)                |
| fullscreen       | false   | Windowed/Fullscreen (ALT + ENTER) |
| reduce_buffering | false   | One Frame-in-Flight               |
| sensitivity      | 2.70f   | Mouse Sensitivity (`0‑100`)       |
| vsync            | false   | Vertical Sync [overrides fps_max] |

#### Example:

```
KitsuneEngine.exe -fps_max 480 -reduce_buffering -fullscreen -sensitivity 2.70
```

---

## Build & Run

### Debug

```bash
cmake -B build/debug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
cmake --build build/debug && build\debug\bin\KitsuneEngine.exe
```

### Release

```bash
cmake -B build/release -G Ninja -DCMAKE_BUILD_TYPE=Release -DSTATIC_BUILD=ON -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
cmake --build build/release && build\release\bin\KitsuneEngine.exe
```
