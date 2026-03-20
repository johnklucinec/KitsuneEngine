# Copyright (c) 2025-2026, Sascha Willems
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.21 FATAL_ERROR)
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
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui)
add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp)
target_include_directories(imgui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${Vulkan_INCLUDE_DIRS})
target_compile_definitions(imgui PUBLIC
    VK_NO_PROTOTYPES
    IMGUI_IMPL_VULKAN_USE_VOLK
    "IMGUI_IMPL_VULKAN_VOLK_FILENAME=<Volk/volk.h>")
target_include_directories(imgui PRIVATE ${CMAKE_SOURCE_DIR}/external)

# Point imgui at SDL3 headers — detected by whether the static target was fetched
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

# --- Executable ---
add_executable(${NAME} main.cpp assets/shader.slang)
target_compile_definitions(${NAME} PRIVATE VK_NO_PROTOTYPES)
if(WIN32)
    target_compile_definitions(${NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()
set_target_properties(${NAME} PROPERTIES DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(${NAME} PRIVATE cxx_std_20)
target_link_libraries(${NAME} PRIVATE Vulkan::Vulkan ${SDL_TARGET} ktx ${Slang_LIBRARY} imgui)
if(NOT WIN32)
    target_link_libraries(${NAME} PRIVATE glm::glm-header-only)
endif()

# --- asset/DLL copying only for static release builds ---
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
