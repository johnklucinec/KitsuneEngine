# Kitsune Engine - A Vulkan Game Engine

This is a _very_ early build.

## Info

- Forward (not Forward+, since less than 5 dynamic lights)
- Burley + GGX
- Overwatch FOV (103deg)
- Overwatch Sensitivity [arg: set sensitivity with -sense <value>]
- Overwatch Movement
- Simple overlay (fps, crosshair, current settings)
- FPS Limiter (Win32 high-res waitable timer + spin-wait, ~1ms precision)

## Immediate Plans

- Convert to ECS Engine

## Semi-Immediate Plans

- Fullscreen Exclusive vs Fullscreen Borderless
- Smart check for all display modes

## Long Term Plans

- Turn into simple FPS Aim Trainer (Like Kovaaks)
- Make FPS Limiter Linux compatable + VK_EXT_present_timing (if that makes sense even)
- Actually understand cmake, make a way to build exe and copy on needed files (release build)

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
