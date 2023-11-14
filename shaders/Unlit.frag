#version 450

#define max_point_light_count 4

struct DirectionalLightRenderResource
{
    vec4 direction;
    vec4 color;
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

layout(std140, set = 1, binding = 0) uniform UBOPerMaterial
{
    vec4 baseColorFactor;
    vec4 emissiveFactor;
	float colorTextureSet;
	float PhysicalDescriptorTextureSet;
    float metallicTextureSet;
    float roughnessTextureSet;
	float normalTextureSet;
	float occlusionTextureSet;
	float emissiveTextureSet;
	float metallicFactor;
	float roughnessFactor;
	float alphaMask;
	float alphaMaskCutoff;
} uboPerMaterial;

layout(set = 1, binding = 1) uniform sampler2D BaseColorSampler;
layout(set = 1, binding = 2) uniform sampler2D MetallicRoughnessSampler;
layout(set = 1, binding = 3) uniform sampler2D MetallicSampler;
layout(set = 1, binding = 4) uniform sampler2D RoughnessSampler;
layout(set = 1, binding = 5) uniform sampler2D NormalSampler;
layout(set = 1, binding = 6) uniform sampler2D AoSampler;
layout(set = 1, binding = 7) uniform sampler2D EmissiveSampler;

layout(std140, set = 2, binding = 0) uniform PerFrameCB
{
    mat4 viewMatrix;
    mat4 projMatrix;
    vec3 camPos;
    float pointLightNumber;
    DirectionalLightRenderResource directionalLit;
    PointLightRenderResource pointLight[max_point_light_count];
} uboPerFrame;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 worldPos;

layout(location = 0) out vec4 outColor;

void main() 
{   
    vec3 BaseColor = uboPerMaterial.colorTextureSet > -1 ? 
        pow(texture(BaseColorSampler, fragTexCoord).rgb, vec3(2.2)) * uboPerMaterial.baseColorFactor.rgb : 
        uboPerMaterial.baseColorFactor.rgb;

    outColor = vec4(BaseColor, 1.0);
}