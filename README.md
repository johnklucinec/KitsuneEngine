# Kitsune Engine - A Vulkan Game Engine

This is a *very* early build.

## Info
* Forward (not Forward+, since less than 5 dynamic lights)
* Burley + GGX 
* Overwatch FOV (103deg)
* Overwatch Sensitivity [arg: set sensitivity with -sense <value>]
* Overwatch Movement
* Simple overlay (fps, crosshair, current settings)

## Immediate Plans
* Add FPS limiter (normal one + one with VK_EXT_present_timing)
* Fullscreen / Windowed mode

## Long Term Plans
* Convert to ECS Engine
* Turn into simple FPS Aim Trainer (Like Kovaaks)

---

## Build & Run

### Debug
```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
cmake --build build && build\\bin\\KitsuneEngine.exe
```

### Release
```bash
cmake -B build/release -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ 
cmake --build build/release
build\\release\\bin\\KitsuneEngine.exe
```
