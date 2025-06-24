#version 450

#define cascade_count 4

layout(set = 0, binding = 0) uniform UBOMeshRenderResource
{
    mat4 modelMatrix;
    mat4 normalMatrix;
} uboPerMesh;

layout(set = 1, binding = 0) uniform UBOCSM
{
    mat4 projViewMatrix;
} uboCSM;

layout(push_constant) uniform PushConstBlock {
    mat4 projViewMat;
} pc;

layout(location = 0) in vec3 inPosition;

void main()
{
    vec4 worldPosV4 = uboPerMesh.modelMatrix * vec4(inPosition, 1.0);
    gl_Position = pc.projViewMat * worldPosV4;
}