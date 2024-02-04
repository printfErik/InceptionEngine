#version 450
#include "PerFrameGlobalUBO.h"

layout(set = 0, binding = 0) uniform UBOMeshRenderResource
{
    mat4 modelMatrix;
    mat4 normalMatrix;
} uboPerMesh;

layout(set = 1, binding = 0) uniform PerFrameCB
{
    mat4 viewMatrix;
    mat4 projMatrix;
    vec3 cameraPos;
    float pointLightNumber;
    float spotLightNumber;
    DirectionalLightRenderResource directionalLit;
    PointLightRenderResource pointLightList[max_point_light_count];
    SpotLightRenderResource spotLightList[max_spot_light_count];
} uboPerFrame;

layout(triangles, invocations = MAX_SPOT_LIGHT_COUNT) in;
layout(triangle_strip, max_vertices = 3) out;

void main()
{
    for (int i = 0; i < gl_in.length(); i++)
	{
        gl_Layer = gl_InvocationID;
        gl_Position = uboPerFrame.spotLightList[gl_InvocationID].VPMatrix * gl_in[i].gl_Positon;
        EmitVertex();
    }
    EndPrimitive();
}

