#version 450

#include "Lighting.h"

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

layout (input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput gBufferA;
layout (input_attachment_index = 1, set = 0, binding = 1) uniform subpassInput gBufferB;
layout (input_attachment_index = 2, set = 0, binding = 2) uniform subpassInput gBufferC;
layout (input_attachment_index = 3, set = 0, binding = 3) uniform subpassInput depthTex;


layout(std140, set = 1, binding = 0) uniform PerFrameCB
{
    mat4 viewMatrix;
    mat4 projMatrix;
    vec3 camPos;
    float pointLightNumber;
    DirectionalLightRenderResource directionalLit;
    PointLightRenderResource pointLight[max_point_light_count];
} uboPerFrame;

layout(location = 0) in vec2 inTexCoord;

layout(location = 0) out vec4 outColor;



void main()
{
    vec4 gbufferA = subpassLoad(gBufferA);
	vec4 gbufferB = subpassLoad(gBufferB);
	vec4 gbufferC = subpassLoad(gBufferC);

	vec3 BaseColor = gbufferA.rgb;
	float Metallic = gbufferA.a;
	vec3 N = gbufferB.rgb;
	float PerceptualRoughness = gbufferB.a;
	vec3 Emissive = gbufferC.rgb;
	float AO = gbufferC.a;
	
	vec4 clipPos = vec4(inTexCoord * 2.0 - 1.0, subpassLoad(depthTex).x, 1.0);
	vec4 worldPos_w = inverse(uboPerFrame.projMatrix * uboPerFrame.viewMatrix) * clipPos;
	vec3 worldPos = worldPos_w.xyz / worldPos_w.w;

	vec3 F0 = vec3(0.04);
    vec3 SpecularColor = mix(F0, BaseColor, Metallic);

    vec3 V = normalize(uboPerFrame.camPos - worldPos);
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
    
    vec3 radiance = uboPerFrame.directionalLit.color.xyz;
    vec3 Lo = (DiffuseContrib + SpecularContrib) * radiance * NdotL;

    vec3 ambient = vec3(0.03) * BaseColor * vec3(AO);
    vec3 color = Lo * vec3(AO) + ambient;

    color += Emissive;

    color = pow(color, vec3(1.0 / 2.2));

    outColor = vec4(1, 0, 1, 1.0);
}