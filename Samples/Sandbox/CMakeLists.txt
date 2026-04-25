
file(GLOB_RECURSE SAMPLES_SOURCES CONFIGURE_DEPENDS
    Source/*.cpp
    Source/*.h
)

add_executable(Sandbox ${SAMPLES_SOURCES})
add_executable(FusionSamples::Sandbox ALIAS Sandbox)

target_link_libraries(Sandbox
    PUBLIC
        Fusion::Core
        Fusion::Platform
        Fusion::SDL3Platform
        Fusion::RHI
        Fusion::VulkanRHI
        Fusion::Widgets
)

set_target_properties(Sandbox PROPERTIES FOLDER "Samples")

fusion_configure_target(Sandbox)
