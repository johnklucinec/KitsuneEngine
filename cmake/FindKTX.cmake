find_path(KTX_INCLUDE_DIR NAMES ktx.h
    HINTS "${CMAKE_SOURCE_DIR}/external/ktx/include"
    REQUIRED)

set(_KTX_SOURCE_DIR "${CMAKE_SOURCE_DIR}/external/ktx/lib")

if(NOT TARGET ktx::ktx)
    add_library(ktx_impl STATIC
        ${_KTX_SOURCE_DIR}/texture.c
        ${_KTX_SOURCE_DIR}/hashlist.c
        ${_KTX_SOURCE_DIR}/checkheader.c
        ${_KTX_SOURCE_DIR}/swap.c
        ${_KTX_SOURCE_DIR}/memstream.c
        ${_KTX_SOURCE_DIR}/filestream.c
        ${_KTX_SOURCE_DIR}/vkloader.c)

    target_include_directories(ktx_impl PUBLIC
        ${KTX_INCLUDE_DIR}
        ${CMAKE_SOURCE_DIR}/external/ktx/other_include
        ${Vulkan_INCLUDE_DIRS})

    add_library(ktx::ktx ALIAS ktx_impl)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(KTX DEFAULT_MSG KTX_INCLUDE_DIR)
