find_package(Vulkan REQUIRED)
set(SHADER_COMPILER "${SHADER_DIR}/glslc.exe" CACHE FILEPATH "Path to glslangValidator")

message(STATUS "Shader compiler: ${SHADER_COMPILER}")

file(GLOB_RECURSE GLSL_SOURCES
    "${SHADER_DIR}/glsl/*.vert"
    "${SHADER_DIR}/glsl/*.frag"
)

set(SPIRV_OUTPUT_DIR "${SHADER_DIR}/spv")
file(MAKE_DIRECTORY "${SPIRV_OUTPUT_DIR}")

set(SPIRV_FILES "")
foreach(shader_src IN LISTS GLSL_SOURCES)
    get_filename_component(fname ${shader_src} NAME_WE)     
    get_filename_component(ext   ${shader_src} EXT)           
    set(out "${SPIRV_OUTPUT_DIR}/${fname}${ext}.spv")

    message(STATUS "shader_src: ${shader_src}")

    add_custom_command(
        OUTPUT "${out}"
        COMMAND ${SHADER_COMPILER}
                "${shader_src}"            
                -o "${out}"
        DEPENDS "${shader_src}"
        COMMENT "Compile GLSL ${shader_src} to SPIR‑V ${out}"
        VERBATIM
    )

    list(APPEND SPIRV_FILES "${out}")
endforeach()

add_custom_target(ShadersSPV ALL
    DEPENDS ${SPIRV_FILES}
)

add_dependencies(${PROJECT_NAME} ShadersSPV)