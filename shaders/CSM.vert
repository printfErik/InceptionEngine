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

layout(set = 1, binding = 0) uniform LightCB
{
    mat4 viewMatrix;
    mat4 projMatrix;
    vec3 cameraPos;
    float pointLightNumber;
    DirectionalLightRenderResource directionalLit;
    PointLightRenderResource pointLight[max_point_light_count];
} uboPerFrame;


layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec3 worldPos;

void main()
{
    
}