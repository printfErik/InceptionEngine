#version 450

#define max_point_light_count 4

struct DirectionalLightRenderResource
{
    vec4 direction;
    vec4 ambient;
};

struct PointLightRenderResource
{
    vec3 position;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	float constant;
	float linear;
	float quadratic;
};

layout(set = 0, binding = 0) uniform UBOMeshRenderResource
{
    mat4 modelMatrix;
    mat4 normalMatrix;
} uboPerMesh;

layout(set = 2, binding = 0) uniform PerFrameCB
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
    fragColor = inColor;
    fragTexCoord = inTexCoord;
    fragNormal = normalize(mat3(uboPerMesh.normalMatrix) * inNormal);
    vec4 worldPosV4 = uboPerMesh.modelMatrix * vec4(inPosition, 1.0);
    worldPos = worldPosV4.rgb;
    gl_Position = uboPerFrame.projMatrix *  uboPerFrame.viewMatrix * worldPosV4;
}