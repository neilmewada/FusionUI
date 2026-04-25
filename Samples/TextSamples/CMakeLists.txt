
file(GLOB_RECURSE SAMPLES_SOURCES CONFIGURE_DEPENDS
    Source/*.cpp
    Source/*.h
)

add_executable(TextSamples ${SAMPLES_SOURCES})
add_executable(FusionSamples::TextSamples ALIAS TextSamples)

target_link_libraries(TextSamples
    PUBLIC
        Fusion::Core
        Fusion::Platform
        Fusion::SDL3Platform
        Fusion::RHI
        Fusion::VulkanRHI
        Fusion::Widgets
)

set_target_properties(TextSamples PROPERTIES FOLDER "Samples")

fusion_configure_target(TextSamples)

