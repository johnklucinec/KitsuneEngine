# Copyright (c) 2025-2026, Johnny Klucinec

cmake_minimum_required(VERSION 3.28 FATAL_ERROR)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
include(FetchContent)

set(NAME KitsuneEngine)
project(${NAME})

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(STATIC_BUILD "Static/portable build for release distribution" OFF)

# --- Vulkan SDK ---
if(NOT IS_DIRECTORY "$ENV{VULKAN_SDK}")
    message(WARNING "Vulkan SDK not detected; set VULKAN_SDK env var or provide libraries manually")
else()
    message(STATUS "Found Vulkan SDK: $ENV{VULKAN_SDK}")
endif()

# --- Global includes ---
include_directories(external/tinyobj)
include_directories(external/ktx/include)
include_directories(external/ktx/other_include)

find_package(Vulkan REQUIRED)

# --- SDL3 ---
if(STATIC_BUILD)
    set(SDL_STATIC ON  CACHE BOOL "" FORCE)
    set(SDL_SHARED OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL
        GIT_TAG        release-3.2.10
        GIT_SHALLOW    ON
        EXCLUDE_FROM_ALL
        SYSTEM)
    FetchContent_MakeAvailable(SDL3)
    set(SDL_TARGET SDL3::SDL3-static)
else()
    find_library(SDL_LIBRARY NAMES SDL3 HINTS "$ENV{VULKAN_SDK}/lib" REQUIRED)
    set(SDL_TARGET ${SDL_LIBRARY})
endif()

# --- GLM ---
if(NOT WIN32)
    FetchContent_Declare(GLM
        GIT_REPOSITORY https://github.com/g-truc/glm
        GIT_TAG 1.0
        GIT_SHALLOW ON
        EXCLUDE_FROM_ALL
        SYSTEM)
    FetchContent_MakeAvailable(GLM)
endif()

# --- EnTT ---
FetchContent_Declare(EnTT
    GIT_REPOSITORY https://github.com/skypjack/entt.git
    GIT_TAG v3.16.0
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_MakeAvailable(EnTT)

# --- KTX ---
set(KTX_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/ktx)
set(KTX_SOURCES
    ${KTX_DIR}/lib/texture.c
    ${KTX_DIR}/lib/hashlist.c
    ${KTX_DIR}/lib/checkheader.c
    ${KTX_DIR}/lib/swap.c
    ${KTX_DIR}/lib/memstream.c
    ${KTX_DIR}/lib/filestream.c
    ${KTX_DIR}/lib/vkloader.c)
add_library(ktx STATIC ${KTX_SOURCES})
target_include_directories(ktx PRIVATE ${Vulkan_INCLUDE_DIRS})

if(APPLE)
    target_link_libraries(ktx PUBLIC $ENV{VULKAN_SDK}/lib/libvulkan.dylib)
elseif(NOT WIN32)
    target_link_libraries(ktx PUBLIC $ENV{VULKAN_SDK}/lib/libvulkan.so)
endif()

find_library(Slang_LIBRARY NAMES slang slang-compiler HINTS "$ENV{VULKAN_SDK}/lib" REQUIRED)

# --- ImGui ---
FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.92.7
    GIT_SHALLOW ON
    DOWNLOAD_ONLY YES)
FetchContent_MakeAvailable(imgui)

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp)
target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${Vulkan_INCLUDE_DIRS})
target_compile_definitions(imgui PUBLIC
    VK_NO_PROTOTYPES
    IMGUI_IMPL_VULKAN_USE_VOLK
    "IMGUI_IMPL_VULKAN_VOLK_FILENAME=<Volk/volk.h>")
target_include_directories(imgui PRIVATE ${CMAKE_SOURCE_DIR}/external)

if(TARGET SDL3::SDL3-static)
    target_link_libraries(imgui PRIVATE SDL3::SDL3-static)
else()
    target_include_directories(imgui PRIVATE $ENV{VULKAN_SDK}/include/SDL3)
endif()

# --- Compiler flags ---
if(WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_WIN32_KHR")
elseif(LINUX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_WAYLAND_KHR")
endif()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNOMINMAX -D_USE_MATH_DEFINES")
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/")

# --- Collect system sources ---
file(GLOB_RECURSE ALL_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_SOURCE_DIR}/src/*.cpp)

# --- Executable ---
add_executable(${NAME}
    assets/shaders/shader.slang
    ${ALL_SOURCES})

target_compile_definitions(${NAME} PRIVATE VK_NO_PROTOTYPES)
if(WIN32)
    target_compile_definitions(${NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# --- Include paths for src layout ---
target_include_directories(${NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/components
    ${CMAKE_SOURCE_DIR}/src/systems
    ${CMAKE_SOURCE_DIR}/src/renderer
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/external
    ${Vulkan_INCLUDE_DIRS})

set_target_properties(${NAME} PROPERTIES DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(${NAME} PRIVATE cxx_std_20)
target_link_libraries(${NAME} PRIVATE
    Vulkan::Vulkan
    ${SDL_TARGET}
    ktx
    ${Slang_LIBRARY}
    imgui
    EnTT::EnTT)
if(NOT WIN32)
    target_link_libraries(${NAME} PRIVATE glm::glm-header-only)
endif()

# --- Asset/DLL copying only for static release builds ---
if(STATIC_BUILD)
    if(WIN32)
        file(GLOB SLANG_DLLS "$ENV{VULKAN_SDK}/Bin/slang*.dll")
        foreach(DLL IN LISTS SLANG_DLLS)
            add_custom_command(TARGET ${NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "${DLL}" $<TARGET_FILE_DIR:${NAME}>
                COMMENT "Copying ${DLL} -> bin/")
        endforeach()
    endif()

    add_custom_command(TARGET ${NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:${NAME}>/assets
        COMMENT "Copying assets -> bin/assets/")
endif()

# --- Copy compile_commands.json to project root ---
add_custom_target(copy-compiledb ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_BINARY_DIR}/compile_commands.json
        ${CMAKE_SOURCE_DIR}/build/compile_commands.json
    COMMENT "Updating compile_commands.json in build/")
