
add_fusion_target(FusionWidgets STATIC
    ALIAS       Fusion::Widgets
    FOLDER      "Modules"
    FILES_CMAKE
        PRIVATE fusionwidgets_private_files.cmake
        PUBLIC  fusionwidgets_public_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Include
    PCHHEADER
        PRIVATE Include/Fusion/Widgets.h
    BUILD_DEPENDENCIES
        PUBLIC
            Fusion::Core
            Fusion::Platform
            Fusion::RHI
            $<$<BOOL:${FUSION_USE_SDL3}>:Fusion::SDL3Platform>
            $<$<BOOL:${FUSION_USE_VULKAN}>:Fusion::VulkanRHI>
            Freetype::Freetype
)

