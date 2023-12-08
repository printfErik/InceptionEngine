#version 450

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
layout(location = 1) out vec4 outGBufferA;
layout(location = 2) out vec4 outGBufferB;
layout(location = 3) out vec4 outGBufferC;

vec3 getNormalFromMap()
{
    vec3 tangentNormal = texture(NormalSampler, fragTexCoord).xyz * 2.0 - 1.0;

    vec3 Q1  = dFdx(worldPos);
    vec3 Q2  = dFdy(worldPos);
    vec2 st1 = dFdx(fragTexCoord);
    vec2 st2 = dFdy(fragTexCoord);

    vec3 N   = normalize(fragNormal);
    vec3 T  = normalize(Q1*st2.t - Q2*st1.t);
    vec3 B  = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}

void main() 
{   
    vec3 BaseColor = uboPerMaterial.colorTextureSet > -1 ? 
        pow(texture(BaseColorSampler, fragTexCoord).rgb, vec3(2.2)) * uboPerMaterial.baseColorFactor.rgb : 
        uboPerMaterial.baseColorFactor.rgb;

    float Metallic = uboPerMaterial.PhysicalDescriptorTextureSet > -1 ?
        texture(MetallicRoughnessSampler, fragTexCoord).g * uboPerMaterial.metallicFactor :
        uboPerMaterial.metallicTextureSet > -1 ? 
            texture(MetallicSampler, fragTexCoord).r * uboPerMaterial.metallicFactor :
            uboPerMaterial.metallicFactor;

    float PerceptualRoughness = uboPerMaterial.PhysicalDescriptorTextureSet > -1 ?
        texture(MetallicRoughnessSampler, fragTexCoord).b * uboPerMaterial.roughnessFactor :
        uboPerMaterial.roughnessTextureSet > -1 ? 
            texture(RoughnessSampler, fragTexCoord).r * uboPerMaterial.roughnessFactor :
            uboPerMaterial.roughnessFactor;

    vec3 Normal = uboPerMaterial.normalTextureSet > -1 ? getNormalFromMap() : normalize(fragNormal);

    float AO = uboPerMaterial.occlusionTextureSet > -1 ? 
        texture(AoSampler, fragTexCoord).r : 1.f;

    vec3 Emissive = uboPerMaterial.emissiveTextureSet > -1 ? 
        pow(texture(EmissiveSampler, fragTexCoord).rgb, vec3(2.2)) * uboPerMaterial.emissiveFactor.rgb :
        uboPerMaterial.emissiveFactor.rgb;

    outGBufferA = vec4(BaseColor, Metallic);
    outGBufferB = vec4(Normal, PerceptualRoughness);
    outGBufferC = vec4(Emissive, AO);

    outColor = vec4(0.f);
}