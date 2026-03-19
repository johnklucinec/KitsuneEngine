# Kitsune Engine - A Vulkan Game Engine

This is a *very* early build.

## Info
* Forward (not Forward+, since less than 5 dynamic lights)
* Burley + GGX 
* Overwatch FOV (103deg)
* Overwatch Sensitivity [arg: set sensitivity with -sense <value>]
* Overwatch Movement
* Simple overlay (fps, crosshair, current settings)
* FPS Limiter (Win32 high-res waitable timer + spin-wait, ~1ms precision)

## Immediate Plans
* Fullscreen / Windowed mode

## Semi-Immediate Plans
* Fullscreen / Windowed mode
* Reduced Buffering setting
* Smart check for all display modes

## Long Term Plans
* Convert to ECS Engine
* Turn into simple FPS Aim Trainer (Like Kovaaks)
* Make FPS Limiter Linux compatable + VK_EXT_present_timing (if that makes sense even)
* Actually understand cmake, make a way to build exe and copy on needed files (release build)

---

## Build & Run

### Debug
```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
cmake --build build && build\\bin\\KitsuneEngine.exe
```

### Release
```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ 
cmake --build build && build\\bin\\KitsuneEngine.exe
```
