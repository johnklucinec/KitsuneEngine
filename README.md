# Kitsune Engine - A Vulkan Game Engine

This is a *very* early build.

# Build & Run

### Debug
```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
cmake --build build
build\\bin\\HowToVulkan.exe
```

### Release
```bash
cmake -B build/release -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ 
cmake --build build/release
build\\release\\bin\\HowToVulkan.exe
```
