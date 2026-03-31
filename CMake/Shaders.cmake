include_guard(GLOBAL)

# fusion_compile_shader(
#   TARGET     FusionShaderLibrary
#   HLSL       Shaders/UI/UIVertex.hlsl
#   ENTRY      VSMain
#   STAGE      vs                          # DXC stage prefix
#   TARGETS    SPIRV MSL                   # which outputs to embed
# )
function(fusion_compile_shader)
    cmake_parse_arguments(ARG "" "TARGET;HLSL;ENTRY;STAGE" "TARGETS" ${ARGN})

    get_filename_component(name ${ARG_HLSL} NAME_WE)
    set(gen_dir "${CMAKE_CURRENT_BINARY_DIR}/Generated/Shaders")
    set(spv     "${gen_dir}/${name}.spv")

    

endfunction()