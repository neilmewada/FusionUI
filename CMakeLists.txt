cmake_minimum_required(VERSION 3.21)

project(FusionUI
    VERSION 0.1.0
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
include(CMake/FusionPlatform.cmake)
include(CMake/Targets.cmake)

if(PROJECT_IS_TOP_LEVEL)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)

    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>")

    option(FUSION_BUILD_TESTS    "Build Fusion tests"               ON)
    option(FUSION_ENABLE_TRACY   "Enable Tracy profiler integration" OFF)
endif()

option(FUSION_BUILD_SAMPLES "Build Fusion tests" ${PROJECT_IS_TOP_LEVEL})

option(FUSION_ENABLE_EXCEPTIONS "Enable exceptions, for safe C++." ON)

include(CMake/ThirdParty.cmake)
include(CMake/Shaders.cmake)

if (FUSION_BUILD_TESTS)
    include(CMake/Tests.cmake)
endif()

add_subdirectory(FusionCore)
add_subdirectory(FusionPlatform)
add_subdirectory(FusionShaderLibrary)

if(FUSION_USE_SDL3)
    add_subdirectory(FusionSDL3Platform)
endif()

add_subdirectory(FusionRHI)

if(FUSION_USE_VULKAN)
    add_subdirectory(FusionVulkanRHI)
endif()

add_subdirectory(FusionWidgets)

if(FUSION_BUILD_SAMPLES AND FUSION_USE_SDL3 AND FUSION_USE_VULKAN)
    add_subdirectory(Samples)
endif()

