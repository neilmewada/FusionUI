
file(GLOB_RECURSE SAMPLES_SOURCES CONFIGURE_DEPENDS
    Source/*.cpp
    Source/*.h
)

add_executable(TreeViewSample ${SAMPLES_SOURCES})
add_executable(FusionSamples::TreeView ALIAS TreeViewSample)

target_link_libraries(TreeViewSample
    PUBLIC
        Fusion::Core
        Fusion::Platform
        Fusion::SDL3Platform
        Fusion::RHI
        Fusion::VulkanRHI
        Fusion::Widgets
)

set_target_properties(TreeViewSample PROPERTIES FOLDER "Samples")

fusion_configure_target(TreeViewSample)

