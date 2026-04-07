include_guard(GLOBAL)

# ---------------------------------------------------------------------------
# Platform detection
# ---------------------------------------------------------------------------
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(FUSION_PLATFORM_WINDOWS 1)
    set(FUSION_PLATFORM_NAME "Windows")
    set(FUSION_TRAIT_VULKAN_SUPPORTED 1)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(FUSION_PLATFORM_MAC 1)
    set(FUSION_PLATFORM_NAME "Mac")
    set(FUSION_TRAIT_METAL_SUPPORTED 1)
    set(FUSION_TRAIT_VULKAN_SUPPORTED 1)  # via MoltenVK
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(FUSION_PLATFORM_LINUX 1)
    set(FUSION_PLATFORM_NAME "Linux")
    set(FUSION_TRAIT_VULKAN_SUPPORTED 1)
else()
    message(FATAL_ERROR "[Fusion] Unsupported platform: ${CMAKE_SYSTEM_NAME}")
endif()

# ---------------------------------------------------------------------------
# Compiler detection
# ---------------------------------------------------------------------------
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(FUSION_COMPILER_MSVC 1)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(FUSION_COMPILER_CLANG 1)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "AppleClang")
    set(FUSION_COMPILER_APPLECLANG 1)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(FUSION_COMPILER_GCC 1)
else()
    message(WARNING "[Fusion] Unrecognized compiler: ${CMAKE_CXX_COMPILER_ID}")
endif()

# ---------------------------------------------------------------------------
# fusion_configure_target(<target>)
#
# Apply Fusion's compile settings to one of Fusion's own targets.
# Do NOT call this on consumer targets — it is for internal use only.
# ---------------------------------------------------------------------------
function(fusion_configure_target target)
    # Platform defines
    target_compile_definitions(${target} PRIVATE
        $<$<BOOL:${FUSION_PLATFORM_WINDOWS}>:FUSION_PLATFORM_WINDOWS=1>
        $<$<BOOL:${FUSION_PLATFORM_MAC}>:FUSION_PLATFORM_MAC=1>
        $<$<BOOL:${FUSION_PLATFORM_LINUX}>:FUSION_PLATFORM_LINUX=1>
        $<$<BOOL:${FUSION_COMPILER_MSVC}>:FUSION_COMPILER_MSVC=1>
        $<$<BOOL:${FUSION_COMPILER_CLANG}>:FUSION_COMPILER_CLANG=1>
        $<$<BOOL:${FUSION_COMPILER_APPLECLANG}>:FUSION_COMPILER_APPLECLANG=1>
        $<$<BOOL:${FUSION_COMPILER_GCC}>:FUSION_COMPILER_GCC=1>
        FUSION_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        FUSION_VERSION_MINOR=${PROJECT_VERSION_MINOR}
        FUSION_VERSION_PATCH=${PROJECT_VERSION_PATCH}
    )

    # Compiler flags
    if(FUSION_COMPILER_MSVC)
        target_compile_options(${target} PRIVATE
            /MP         # parallel compilation
            /W4         # warning level 4
            /wd4996     # suppress deprecation
        )
    elseif(FUSION_COMPILER_CLANG OR FUSION_COMPILER_GCC OR FUSION_COMPILER_APPLECLANG)
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

function(fusion_filter_platform_files FILES_LIST)
    if(NOT ${FUSION_PLATFORM_NAME} STREQUAL "Windows")
        list(FILTER ${FILES_LIST} EXCLUDE REGEX ".*/PAL/Windows/.*")
    endif()
    
    if(NOT ${FUSION_PLATFORM_NAME} STREQUAL "Mac")
        list(FILTER ${FILES_LIST} EXCLUDE REGEX ".*/PAL/Mac/.*")
    endif()

    if(NOT ${FUSION_PLATFORM_NAME} STREQUAL "Linux")
        list(FILTER ${FILES_LIST} EXCLUDE REGEX ".*/PAL/Linux/.*")
    endif()

    if(NOT ${FUSION_PLATFORM_NAME} STREQUAL "Android")
        list(FILTER ${FILES_LIST} EXCLUDE REGEX ".*/PAL/Android/.*")
    endif()

    if(NOT ${FUSION_PLATFORM_NAME} STREQUAL "iOS")
        list(FILTER ${FILES_LIST} EXCLUDE REGEX ".*/PAL/iOS/.*")
    endif()
    
    set(${FILES_LIST} ${${FILES_LIST}} PARENT_SCOPE)
endfunction()
