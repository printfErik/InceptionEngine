#version 450

struct DirectionalLightRenderResource
{
    vec4 direction;
    vec4 ambient;
};

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
    DirectionalLightRenderResource directionalLit;
    PointLightRenderResource pointLight[max_point_light_count];
} uboPerFrame;