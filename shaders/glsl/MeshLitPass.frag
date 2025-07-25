#version 450

#include "Lighting.h"
#include "PerFrameGlobalUBO.h"

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
    vec3 cameraPos;
    float pointLightNumber;
    float spotLightNumber;
    DirectionalLightRenderResource directionalLit;
} uboPerFrame;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 worldPos;

layout(location = 0) out vec4 outColor;

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
    
    //float AlphaRoughness = PerceptualRoughness * PerceptualRoughness;

    vec3 F0 = vec3(0.04);
    vec3 SpecularColor = mix(F0, BaseColor, Metallic);

    vec3 N = uboPerMaterial.normalTextureSet > -1 ? getNormalFromMap() : normalize(fragNormal);
    vec3 V = normalize(uboPerFrame.cameraPos - worldPos);
    vec3 L = normalize(uboPerFrame.directionalLit.direction.xyz);
    vec3 H = normalize(V + L);

    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    float NdotH = clamp(dot(N, H), 0.0, 1.0);
    float LdotH = clamp(dot(L, H), 0.0, 1.0);
    float VdotH = max(dot(V, H), 0.0);
    
    float NDF = DistributionGGX(N, H, PerceptualRoughness);
    float G = GeometrySmith(N, V, L, PerceptualRoughness);
    vec3 F = fresnelSchlick(VdotH, SpecularColor);

    vec3 numerator = NDF * G * F; 
    float denominator = 4.0 * NdotV * NdotL + 0.0001;
    vec3 SpecularContrib = numerator / denominator;

    vec3 DiffuseColor = BaseColor * (vec3(1.0) - F0) * (1.0 - Metallic);
    vec3 DiffuseContrib = (vec3(1.0) - F) * DiffuseColor / PI;
    
    vec3 radiance = uboPerFrame.directionalLit.ambient.xyz;
    vec3 Lo = (DiffuseContrib + SpecularContrib) * radiance * NdotL;

    // if AO texture exists
    float AO = uboPerMaterial.occlusionTextureSet > -1 ? 
        texture(AoSampler, fragTexCoord).r : 1.f;

    vec3 ambient = vec3(0.03) * BaseColor * vec3(AO);
    vec3 color = Lo * vec3(AO) + ambient;

    vec3 Emissive = uboPerMaterial.emissiveTextureSet > -1 ? 
        pow(texture(EmissiveSampler, fragTexCoord).rgb, vec3(2.2)) * uboPerMaterial.emissiveFactor.rgb :
        uboPerMaterial.emissiveFactor.rgb;

    color += Emissive;

    color = pow(color, vec3(1.0 / 2.2));

    outColor = vec4(color, 1.0);
}