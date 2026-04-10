
add_fusion_target(FusionVulkanRHI STATIC
    ALIAS       Fusion::VulkanRHI
    FOLDER      "Modules"
    FILES_CMAKE
        PRIVATE fusionvulkanrhi_private_files.cmake
        PUBLIC  fusionvulkanrhi_public_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Include
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Source
    COMPILE_DEFINITIONS
        PRIVATE $<$<BOOL:${FUSION_SHADER_DEBUG_SYMBOLS}>:FUSION_SHADER_DEBUG_SYMBOLS=1>
    PCHHEADER
        PRIVATE Source/VulkanRHIPrivate.h
    BUILD_DEPENDENCIES
        PUBLIC
            Fusion::Core
            Fusion::Platform
            Fusion::RHI
            Vulkan::Vulkan
)

if(FUSION_PLATFORM_MAC)
    # ObjC++ .mm files cannot share the C++ PCH
    set_source_files_properties(
        Source/PAL/Mac/VulkanMacPlatform.mm
        PROPERTIES SKIP_PRECOMPILE_HEADERS ON
    )

    target_link_libraries(FusionVulkanRHI PRIVATE
        "-framework Cocoa"
        "-framework QuartzCore"
    )
endif()

