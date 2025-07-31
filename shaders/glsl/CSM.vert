#version 450

#define cascade_count 4

layout(set = 0, binding = 0) uniform UBOMeshRenderResource
{
    mat4 modelMatrix;
    mat4 normalMatrix;
} uboPerMesh;

layout(push_constant) uniform PushConstBlock {
    mat4 projViewMat;
} pc;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inTexCoord;

void main()
{
    vec4 worldPosV4 = uboPerMesh.modelMatrix * vec4(inPosition, 1.0);
    gl_Position = pc.projViewMat * worldPosV4;
}