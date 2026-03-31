include_guard(GLOBAL)

# ---------------------------------------------------------------------------
# Find slangc -- bundled with Vulkan SDK 1.3.268+
# ---------------------------------------------------------------------------
if(Vulkan_GLSLC_EXECUTABLE)
    get_filename_component(_vulkan_bin_dir "${Vulkan_GLSLC_EXECUTABLE}" DIRECTORY)
endif()

find_program(FUSION_SLANGC
    NAMES slangc
    HINTS "${_vulkan_bin_dir}" "$ENV{VULKAN_SDK}/Bin" "$ENV{VULKAN_SDK}/bin"
)

if(NOT FUSION_SLANGC)
    message(WARNING
        "[Fusion] slangc not found -- HLSL shader compilation is unavailable. "
        "Install Vulkan SDK 1.3.268+ and ensure it is on PATH.")
endif()

# ---------------------------------------------------------------------------
# fusion_compile_shader(
#   TARGET  <target>
#   HLSL    <path relative to CMAKE_CURRENT_SOURCE_DIR>
#   GROUP   <name shared by all stages of this shader, used as C++ identifier>
#   ENTRY   <entry point function name>
#   STAGE   <vertex|fragment|compute|geometry|hull|domain>
#   TARGETS <SPIRV> [MSL]
# )
#
# Multiple calls with the same GROUP are grouped into one FShader at runtime.
# Compiles the shader to the requested format(s) at build time and embeds
# the result as a C++ byte array compiled into the target.
# ---------------------------------------------------------------------------
function(fusion_compile_shader)
    cmake_parse_arguments(ARG "" "TARGET;HLSL;GROUP;ENTRY;STAGE" "TARGETS" ${ARGN})

    if(NOT ARG_TARGET OR NOT ARG_HLSL OR NOT ARG_GROUP OR NOT ARG_ENTRY OR NOT ARG_STAGE)
        message(FATAL_ERROR "[Fusion] fusion_compile_shader: TARGET, HLSL, GROUP, ENTRY, and STAGE are all required.")
    endif()

    if(NOT FUSION_SLANGC)
        message(WARNING "[Fusion] Skipping shader '${ARG_GROUP}' (slangc not found).")
        return()
    endif()

    # Internal variable name: GROUP_stage (e.g. Fusion_vertex)
    set(var_name     "${ARG_GROUP}_${ARG_STAGE}")
    set(hlsl_abs     "${CMAKE_CURRENT_SOURCE_DIR}/${ARG_HLSL}")
    set(gen_dir      "${CMAKE_CURRENT_BINARY_DIR}/Generated/Shaders")
    set(embed_script "${PROJECT_SOURCE_DIR}/CMake/Scripts/EmbedShader.cmake")

    # ---- SPIR-V ------------------------------------------------------------
    if("SPIRV" IN_LIST ARG_TARGETS)
        set(spv_file "${gen_dir}/${var_name}.spv")
        set(spv_cpp  "${gen_dir}/${var_name}_SPIRV.cpp")

        add_custom_command(
            OUTPUT  "${spv_file}"
            COMMAND "${FUSION_SLANGC}" "${hlsl_abs}"
                    -target spirv
                    -entry  "${ARG_ENTRY}"
                    -stage  "${ARG_STAGE}"
                    -o      "${spv_file}"
            DEPENDS "${hlsl_abs}"
            COMMENT "[Fusion] slangc: ${var_name} -> SPIR-V"
            VERBATIM
        )

        add_custom_command(
            OUTPUT  "${spv_cpp}"
            COMMAND "${CMAKE_COMMAND}"
                    "-DINPUT=${spv_file}"
                    "-DOUTPUT=${spv_cpp}"
                    "-DVAR_NAME=${var_name}"
                    "-DFORMAT=SPIRV"
                    -P "${embed_script}"
            DEPENDS "${spv_file}" "${embed_script}"
            COMMENT "[Fusion] Embed: ${var_name}.spv -> C++"
            VERBATIM
        )

        target_sources(${ARG_TARGET} PRIVATE "${spv_cpp}")
    endif()

    # ---- MSL ---------------------------------------------------------------
    if("MSL" IN_LIST ARG_TARGETS)
        set(msl_file "${gen_dir}/${var_name}.msl")
        set(msl_cpp  "${gen_dir}/${var_name}_MSL.cpp")

        add_custom_command(
            OUTPUT  "${msl_file}"
            COMMAND "${FUSION_SLANGC}" "${hlsl_abs}"
                    -target msl
                    -entry  "${ARG_ENTRY}"
                    -stage  "${ARG_STAGE}"
                    -o      "${msl_file}"
            DEPENDS "${hlsl_abs}"
            COMMENT "[Fusion] slangc: ${var_name} -> MSL"
            VERBATIM
        )

        add_custom_command(
            OUTPUT  "${msl_cpp}"
            COMMAND "${CMAKE_COMMAND}"
                    "-DINPUT=${msl_file}"
                    "-DOUTPUT=${msl_cpp}"
                    "-DVAR_NAME=${var_name}"
                    "-DFORMAT=MSL"
                    -P "${embed_script}"
            DEPENDS "${msl_file}" "${embed_script}"
            COMMENT "[Fusion] Embed: ${var_name}.msl -> C++"
            VERBATIM
        )

        target_sources(${ARG_TARGET} PRIVATE "${msl_cpp}")
    endif()

    # ---- Record for registry generation ------------------------------------
    set(has_spirv FALSE)
    set(has_msl   FALSE)
    if("SPIRV" IN_LIST ARG_TARGETS)
        set(has_spirv TRUE)
    endif()
    if("MSL" IN_LIST ARG_TARGETS)
        set(has_msl TRUE)
    endif()

    # Descriptor format: GROUP|STAGE|ENTRY|HAS_SPIRV|HAS_MSL
    set(descriptor "${ARG_GROUP}|${ARG_STAGE}|${ARG_ENTRY}|${has_spirv}|${has_msl}")
    set_property(TARGET ${ARG_TARGET} APPEND PROPERTY FUSION_SHADER_LIST "${descriptor}")
endfunction()

# ---------------------------------------------------------------------------
# fusion_finalize_shader_registry(<target>)
#
# Call once after all fusion_compile_shader() invocations for a target.
# Generates ShaderRegistry.cpp which groups stages into FShader objects and
# implements All() and FindShader().
# ---------------------------------------------------------------------------
function(fusion_finalize_shader_registry target)
    get_target_property(shader_list ${target} FUSION_SHADER_LIST)

    set(gen_dir      "${CMAKE_CURRENT_BINARY_DIR}/Generated")
    set(registry_cpp "${gen_dir}/ShaderRegistry.cpp")

    set(src "// Auto-generated by FusionShaderLibrary CMake -- do not edit.\n")
    string(APPEND src "#include \"Fusion/ShaderLibrary/EmbeddedShaders.h\"\n")
    string(APPEND src "#include <cstring>\n\n")

    if(NOT shader_list)
        # No shaders registered yet -- generate stubs so All() and FindShader() link.
        string(APPEND src "namespace Fusion::Shaders {\n\n")
        string(APPEND src "FShaderView    All()                      { return {}; }\n")
        string(APPEND src "const FShader* FindShader(const char*)    { return nullptr; }\n\n")
        string(APPEND src "} // namespace Fusion::Shaders\n")
    else()
        # -- Forward-declare all embedded data accessors ----------------------
        string(APPEND src "namespace Fusion::Shaders::Embedded {\n")
        foreach(desc ${shader_list})
            string(REPLACE "|" ";" parts "${desc}")
            list(GET parts 0 group)
            list(GET parts 1 stage)
            list(GET parts 3 has_spirv)
            list(GET parts 4 has_msl)
            set(var "${group}_${stage}")
            if(has_spirv)
                string(APPEND src "    extern const Fusion::u8* ${var}_SPIRV();\n")
                string(APPEND src "    extern Fusion::SizeT     ${var}_SPIRVSize();\n")
            endif()
            if(has_msl)
                string(APPEND src "    extern const char*       ${var}_MSL();\n")
                string(APPEND src "    extern Fusion::SizeT     ${var}_MSLSize();\n")
            endif()
        endforeach()
        string(APPEND src "} // namespace Fusion::Shaders::Embedded\n\n")

        # -- Collect unique groups (preserving registration order) ------------
        set(groups "")
        foreach(desc ${shader_list})
            string(REPLACE "|" ";" parts "${desc}")
            list(GET parts 0 group)
            if(NOT group IN_LIST groups)
                list(APPEND groups "${group}")
            endif()
        endforeach()

        string(APPEND src "namespace Fusion::Shaders {\n\n")

        # -- Per-group module arrays ------------------------------------------
        foreach(group ${groups})
            string(APPEND src "static const FShaderModule s_${group}_Modules[] = {\n")

            foreach(desc ${shader_list})
                string(REPLACE "|" ";" parts "${desc}")
                list(GET parts 0 g)
                if(NOT g STREQUAL group)
                    continue()
                endif()

                list(GET parts 1 stage_str)
                list(GET parts 2 entry)
                list(GET parts 3 has_spirv)
                list(GET parts 4 has_msl)
                set(var "${group}_${stage_str}")

                if(stage_str     STREQUAL "vertex")
                    set(stage_enum "FShaderStage::Vertex")
                elseif(stage_str STREQUAL "fragment")
                    set(stage_enum "FShaderStage::Fragment")
                elseif(stage_str STREQUAL "compute")
                    set(stage_enum "FShaderStage::Compute")
                elseif(stage_str STREQUAL "geometry")
                    set(stage_enum "FShaderStage::Geometry")
                elseif(stage_str STREQUAL "hull")
                    set(stage_enum "FShaderStage::TessControl")
                elseif(stage_str STREQUAL "domain")
                    set(stage_enum "FShaderStage::TessEval")
                else()
                    message(WARNING "[Fusion] Unknown stage '${stage_str}' in group '${group}'.")
                    set(stage_enum "FShaderStage::Vertex")
                endif()

                if(has_spirv)
                    set(spirv_data "Embedded::${var}_SPIRV()")
                    set(spirv_size "Embedded::${var}_SPIRVSize()")
                else()
                    set(spirv_data "nullptr")
                    set(spirv_size "0")
                endif()

                if(has_msl)
                    set(msl_data "Embedded::${var}_MSL()")
                    set(msl_size "Embedded::${var}_MSLSize()")
                else()
                    set(msl_data "nullptr")
                    set(msl_size "0")
                endif()

                string(APPEND src "    { ${stage_enum}, \"${entry}\", ${spirv_data}, ${spirv_size}, ${msl_data}, ${msl_size} },\n")
            endforeach()

            string(APPEND src "};\n\n")
        endforeach()

        # -- Shader table ----------------------------------------------------
        string(APPEND src "static const FShader s_Shaders[] = {\n")
        foreach(group ${groups})
            # Count modules for this group
            set(module_count 0)
            foreach(desc ${shader_list})
                string(REPLACE "|" ";" parts "${desc}")
                list(GET parts 0 g)
                if(g STREQUAL group)
                    math(EXPR module_count "${module_count} + 1")
                endif()
            endforeach()
            string(APPEND src "    { \"${group}\", s_${group}_Modules, ${module_count} },\n")
        endforeach()
        string(APPEND src "};\n\n")

        # -- All() and FindShader() ------------------------------------------
        string(APPEND src "FShaderView All() {\n")
        string(APPEND src "    return { s_Shaders, sizeof(s_Shaders) / sizeof(s_Shaders[0]) };\n")
        string(APPEND src "}\n\n")

        string(APPEND src "const FShader* FindShader(const char* name) {\n")
        string(APPEND src "    for (const auto& s : s_Shaders)\n")
        string(APPEND src "        if (std::strcmp(s.Name, name) == 0) return &s;\n")
        string(APPEND src "    return nullptr;\n")
        string(APPEND src "}\n\n")

        string(APPEND src "} // namespace Fusion::Shaders\n")
    endif()

    file(GENERATE OUTPUT "${registry_cpp}" CONTENT "${src}")
    target_sources(${target} PRIVATE "${registry_cpp}")
    target_include_directories(${target} PRIVATE "${gen_dir}")
endfunction()
