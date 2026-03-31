
add_fusion_target(FusionShaderLibrary STATIC
    ALIAS       Fusion::ShaderLibrary
    FOLDER      "Modules"
    FILES_CMAKE
        PRIVATE fusionshaderlibrary_private_files.cmake
        PUBLIC  fusionshaderlibrary_public_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Include
    PCHHEADER
        PRIVATE Include/Fusion/ShaderLibrary.h
    BUILD_DEPENDENCIES
        PUBLIC
            Fusion::Core
)
