if(INCEPTION_RENDER_BACKEND STREQUAL "D3D12")
    set(DXC_HINT_DIRS "$ENV{DXC_PATH}" "$ENV{VULKAN_SDK}/Bin")
    if(WIN32)
        file(GLOB DXC_WINDOWS_KIT_DIRS
            "$ENV{ProgramFiles\(x86\)}/Windows Kits/10/bin/*/x64"
            "$ENV{ProgramFiles}/Windows Kits/10/bin/*/x64")
        list(APPEND DXC_HINT_DIRS ${DXC_WINDOWS_KIT_DIRS})
    endif()

    find_program(DXC_EXECUTABLE NAMES dxc dxc.exe HINTS ${DXC_HINT_DIRS})
    if(NOT DXC_EXECUTABLE)
        message(FATAL_ERROR "dxc is required for the D3D12 backend. Install Windows SDK or set DXC_PATH to the directory containing dxc.exe.")
    endif()

    set(DXIL_OUTPUT_DIR "${SHADER_DIR}/dxil")
    file(MAKE_DIRECTORY "${DXIL_OUTPUT_DIR}")

    set(HLSL_OUTPUTS
        "${DXIL_OUTPUT_DIR}/CSMVS.cso"
        "${DXIL_OUTPUT_DIR}/CSMPS.cso"
        "${DXIL_OUTPUT_DIR}/GBufferVS.cso"
        "${DXIL_OUTPUT_DIR}/GBufferPS.cso"
        "${DXIL_OUTPUT_DIR}/DeferredCompositeVS.cso"
        "${DXIL_OUTPUT_DIR}/DeferredCompositePS.cso"
        "${DXIL_OUTPUT_DIR}/GTAOCS.cso"
        "${DXIL_OUTPUT_DIR}/TranslucentVS.cso"
        "${DXIL_OUTPUT_DIR}/TranslucentPS.cso"
    )

    add_custom_command(
        OUTPUT "${DXIL_OUTPUT_DIR}/CSMVS.cso"
        COMMAND ${DXC_EXECUTABLE} -T vs_6_0 -E VSMain "${SHADER_DIR}/hlsl/CSM.hlsl" -Fo "${DXIL_OUTPUT_DIR}/CSMVS.cso"
        DEPENDS "${SHADER_DIR}/hlsl/CSM.hlsl"
        VERBATIM)
    add_custom_command(
        OUTPUT "${DXIL_OUTPUT_DIR}/CSMPS.cso"
        COMMAND ${DXC_EXECUTABLE} -T ps_6_0 -E PSMain "${SHADER_DIR}/hlsl/CSM.hlsl" -Fo "${DXIL_OUTPUT_DIR}/CSMPS.cso"
        DEPENDS "${SHADER_DIR}/hlsl/CSM.hlsl"
        VERBATIM)

    add_custom_command(
        OUTPUT "${DXIL_OUTPUT_DIR}/GBufferVS.cso"
        COMMAND ${DXC_EXECUTABLE} -T vs_6_0 -E VSMain "${SHADER_DIR}/hlsl/GBuffer.hlsl" -Fo "${DXIL_OUTPUT_DIR}/GBufferVS.cso"
        DEPENDS "${SHADER_DIR}/hlsl/GBuffer.hlsl"
        VERBATIM)
    add_custom_command(
        OUTPUT "${DXIL_OUTPUT_DIR}/GBufferPS.cso"
        COMMAND ${DXC_EXECUTABLE} -T ps_6_0 -E PSMain "${SHADER_DIR}/hlsl/GBuffer.hlsl" -Fo "${DXIL_OUTPUT_DIR}/GBufferPS.cso"
        DEPENDS "${SHADER_DIR}/hlsl/GBuffer.hlsl"
        VERBATIM)
    add_custom_command(
        OUTPUT "${DXIL_OUTPUT_DIR}/DeferredCompositeVS.cso"
        COMMAND ${DXC_EXECUTABLE} -T vs_6_0 -E VSMain "${SHADER_DIR}/hlsl/DeferredComposite.hlsl" -Fo "${DXIL_OUTPUT_DIR}/DeferredCompositeVS.cso"
        DEPENDS "${SHADER_DIR}/hlsl/DeferredComposite.hlsl"
        VERBATIM)
    add_custom_command(
        OUTPUT "${DXIL_OUTPUT_DIR}/DeferredCompositePS.cso"
        COMMAND ${DXC_EXECUTABLE} -T ps_6_0 -E PSMain "${SHADER_DIR}/hlsl/DeferredComposite.hlsl" -Fo "${DXIL_OUTPUT_DIR}/DeferredCompositePS.cso"
        DEPENDS "${SHADER_DIR}/hlsl/DeferredComposite.hlsl" "${SHADER_DIR}/hlsl/Lighting.hlsli"
        VERBATIM)
    add_custom_command(
        OUTPUT "${DXIL_OUTPUT_DIR}/GTAOCS.cso"
        COMMAND ${DXC_EXECUTABLE} -T cs_6_0 -E CSMain "${SHADER_DIR}/hlsl/GTAO.hlsl" -Fo "${DXIL_OUTPUT_DIR}/GTAOCS.cso"
        DEPENDS "${SHADER_DIR}/hlsl/GTAO.hlsl"
        VERBATIM)
    add_custom_command(
        OUTPUT "${DXIL_OUTPUT_DIR}/TranslucentVS.cso"
        COMMAND ${DXC_EXECUTABLE} -T vs_6_0 -E VSMain "${SHADER_DIR}/hlsl/Translucent.hlsl" -Fo "${DXIL_OUTPUT_DIR}/TranslucentVS.cso"
        DEPENDS "${SHADER_DIR}/hlsl/Translucent.hlsl" "${SHADER_DIR}/hlsl/Lighting.hlsli"
        VERBATIM)
    add_custom_command(
        OUTPUT "${DXIL_OUTPUT_DIR}/TranslucentPS.cso"
        COMMAND ${DXC_EXECUTABLE} -T ps_6_0 -E PSMain "${SHADER_DIR}/hlsl/Translucent.hlsl" -Fo "${DXIL_OUTPUT_DIR}/TranslucentPS.cso"
        DEPENDS "${SHADER_DIR}/hlsl/Translucent.hlsl" "${SHADER_DIR}/hlsl/Lighting.hlsli"
        VERBATIM)

    add_custom_target(ShadersDXIL ALL DEPENDS ${HLSL_OUTPUTS})
    add_dependencies(${PROJECT_NAME} ShadersDXIL)
else()
    find_package(Vulkan REQUIRED)
    set(SHADER_COMPILER "${SHADER_DIR}/glslc.exe" CACHE FILEPATH "Path to glslangValidator")

    message(STATUS "Shader compiler: ${SHADER_COMPILER}")

    file(GLOB_RECURSE GLSL_SOURCES
        "${SHADER_DIR}/glsl/*.vert"
        "${SHADER_DIR}/glsl/*.frag"
        "${SHADER_DIR}/glsl/*.comp"
    )

    set(SPIRV_OUTPUT_DIR "${SHADER_DIR}/spv")
    file(MAKE_DIRECTORY "${SPIRV_OUTPUT_DIR}")

    set(SPIRV_FILES "")
    foreach(shader_src IN LISTS GLSL_SOURCES)
        get_filename_component(fname ${shader_src} NAME_WE)
        get_filename_component(ext ${shader_src} EXT)
        set(out "${SPIRV_OUTPUT_DIR}/${fname}${ext}.spv")

        add_custom_command(
            OUTPUT "${out}"
            COMMAND ${SHADER_COMPILER} "${shader_src}" -o "${out}"
            DEPENDS "${shader_src}"
            VERBATIM)

        list(APPEND SPIRV_FILES "${out}")
    endforeach()

    add_custom_target(ShadersSPV ALL DEPENDS ${SPIRV_FILES})
    add_dependencies(${PROJECT_NAME} ShadersSPV)
endif()
