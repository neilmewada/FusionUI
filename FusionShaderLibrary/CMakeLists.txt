
add_fusion_target(FusionShaderLibrary STATIC
    ALIAS       Fusion::ShaderLibrary
    FOLDER      "Modules"
    FILES_CMAKE
        PRIVATE fusionshaderlibrary_private_files.cmake
        PUBLIC  fusionshaderlibrary_public_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Include
    BUILD_DEPENDENCIES
        PUBLIC
            Fusion::Core
)

# ---------------------------------------------------------------------------
# Shaders
# ---------------------------------------------------------------------------

fusion_compile_shader(TARGET FusionShaderLibrary
    HLSL    Shaders/Fusion.slang
    GROUP   Fusion
    ENTRY   VSMain
    STAGE   vertex
    TARGETS SPIRV
)

fusion_compile_shader(TARGET FusionShaderLibrary
    HLSL    Shaders/Fusion.slang
    GROUP   Fusion
    ENTRY   PSMain
    STAGE   fragment
    TARGETS SPIRV
)

fusion_finalize_shader_registry(FusionShaderLibrary)
