#version 450

#define max_point_light_count 16

struct DirectionalLightRenderResource
{
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
}

struct PointLightRenderResource
{
    vec3 position;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	float constant;
	float linear;
	float quadratic;
}

layout(set = 0, binding = 0) uniform UBOMeshRenderResource
{
    mat4 modelMatrix;
} uboPerMesh;

layout(set = 1, binding = 0) uniform UBOPerMaterial
{
    float shininess;
} uboPerMaterial;

layout(set = 2, binding = 0) readonly buffer SSBOPerFrame
{
    mat4 viewMatrix;
    mat4 projMatrix;
    DirectionalLightRenderResource directionalLit;
    PointLightRenderResource pointLig[max_point_light_count]
}

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNormal;

void main()
{
    gl_Position = projMatrix * viewMatrix * uboPerMesh.modelMatrix * vec4(inPosition, 1.0);
    fragColor = inColor;
    fragTexCoord = inTexCoord;

    mat3 tangentMatrix = mat3(uboPerMesh.modelMatrix[0].xyz, uboPerMesh.modelMatrix[1].xyz, uboPerMesh.modelMatrix[2].xyz);

    fragNormal = normalize(tangentMatrix * inNormal);
}