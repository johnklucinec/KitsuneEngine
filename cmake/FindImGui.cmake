include(FetchContent)
FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.92.7
    GIT_SHALLOW    ON
    DOWNLOAD_ONLY  YES)
FetchContent_MakeAvailable(imgui)

if(NOT TARGET imgui::imgui)
    add_library(imgui_impl STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp)
    target_include_directories(imgui_impl PUBLIC
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends)
    add_library(imgui::imgui ALIAS imgui_impl)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ImGui DEFAULT_MSG imgui_SOURCE_DIR)
