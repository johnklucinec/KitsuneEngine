cmake_minimum_required(VERSION 3.28)
project(KitsuneEngine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

file(TO_CMAKE_PATH "$ENV{VULKAN_SDK}" VULKAN_SDK_PATH)

# --- Dependencies ---
find_package(Vulkan   REQUIRED)
find_package(glm      REQUIRED)
find_package(EnTT     REQUIRED)
find_package(fastgltf REQUIRED)
find_package(KTX      REQUIRED)
find_package(ImGui    REQUIRED)

# --- SDL3 from Vulkan SDK ---
find_library(SDL3_LIB     NAMES SDL3 HINTS "${VULKAN_SDK_PATH}/lib" REQUIRED)
find_path   (SDL3_INCLUDE NAMES SDL3/SDL.h HINTS "${VULKAN_SDK_PATH}/include" REQUIRED)
add_library(SDL3::SDL3 SHARED IMPORTED GLOBAL)
set_target_properties(SDL3::SDL3 PROPERTIES
				IMPORTED_IMPLIB "${SDL3_LIB}"
				INTERFACE_INCLUDE_DIRECTORIES "${SDL3_INCLUDE}")

# --- Slang from Vulkan SDK ---
find_library(Slang_LIBRARY NAMES slang HINTS "${VULKAN_SDK_PATH}/lib" REQUIRED)

# --- ImGui Volk defines ---
target_compile_definitions(imgui_impl PUBLIC
				VK_NO_PROTOTYPES
				IMGUI_IMPL_VULKAN_USE_VOLK
				"IMGUI_IMPL_VULKAN_VOLK_FILENAME=<Volk/volk.h>")
target_link_libraries(imgui_impl PRIVATE SDL3::SDL3 Vulkan::Vulkan)

# --- KitsuneEngine ---
file(GLOB_RECURSE ALL_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/*.cpp)
add_executable(KitsuneEngine ${ALL_SOURCES})

target_compile_definitions(KitsuneEngine PRIVATE
				VK_NO_PROTOTYPES NOMINMAX _USE_MATH_DEFINES
				$<$<BOOL:${WIN32}>:VK_USE_PLATFORM_WIN32_KHR _CRT_SECURE_NO_WARNINGS>
				$<$<AND:$<NOT:$<BOOL:${WIN32}>>,$<NOT:$<BOOL:${APPLE}>>>:VK_USE_PLATFORM_WAYLAND_KHR>)

if(MSVC)
	target_compile_options(KitsuneEngine PRIVATE /EHsc /std:c++latest)
endif()

target_include_directories(KitsuneEngine PRIVATE
				${CMAKE_SOURCE_DIR}/src
				${CMAKE_SOURCE_DIR}/external
				${Vulkan_INCLUDE_DIRS})

target_link_libraries(KitsuneEngine PRIVATE
				Vulkan::Vulkan SDL3::SDL3 glm::glm
				ktx::ktx ${Slang_LIBRARY} imgui::imgui
				EnTT::EnTT fastgltf::fastgltf
				$<$<BOOL:${WIN32}>:winmm>)
