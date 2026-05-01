#pragma pack_matrix(column_major)

#include "Lighting.hlsli"

struct DirectionalLightRenderResource
{
	float4 direction;
	float4 color;
};

struct PointLightRenderResource
{
	float4x4 viewMatrices[6];
	float4 color;
	float3 position;
	float padding0;
};

cbuffer PerFrameCB : register(b0, space2)
{
	float4x4 viewMatrix;
	float4x4 projMatrix;
	float4x4 invViewProjMatrix;
	float3 cameraPos;
	float pointLightNumber;
	DirectionalLightRenderResource directionalLit;
	PointLightRenderResource pointLight[4];
};

Texture2D GBufferA : register(t0, space0);
Texture2D GBufferB : register(t1, space0);
Texture2D GBufferC : register(t2, space0);
Texture2D DepthTex : register(t3, space0);
SamplerState LinearSampler : register(s0, space0);

struct VSOutput
{
	float4 position : SV_POSITION;
	float2 texCoord : TEXCOORD0;
};

VSOutput VSMain(uint vertexId : SV_VertexID)
{
	VSOutput output;
	output.texCoord = float2((vertexId << 1) & 2, vertexId & 2);
	output.position = float4(output.texCoord * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
	return output;
}

float3 ReconstructWorldPosition(float2 uv, float depth)
{
	float2 ndc = uv * 2.0f - 1.0f;
	ndc.y = -ndc.y;
	float4 clip = float4(ndc, depth, 1.0f);
	float4 world = mul(invViewProjMatrix, clip);
	return world.xyz / world.w;
}

float4 PSMain(VSOutput input) : SV_Target0
{
	float4 gbufferA = GBufferA.Sample(LinearSampler, input.texCoord);
	float4 gbufferB = GBufferB.Sample(LinearSampler, input.texCoord);
	float4 gbufferC = GBufferC.Sample(LinearSampler, input.texCoord);
	float depth = DepthTex.Sample(LinearSampler, input.texCoord).r;

	float3 baseColor = gbufferA.rgb;
	float metallic = gbufferA.a;
	float3 normal = normalize(gbufferB.rgb);
	float perceptualRoughness = max(gbufferB.a, 0.04f);
	float3 emissive = gbufferC.rgb;
	float ao = gbufferC.a;
	float3 worldPos = ReconstructWorldPosition(input.texCoord, depth);

	float3 f0 = lerp(float3(0.04f, 0.04f, 0.04f), baseColor, metallic);
	float3 viewDir = normalize(cameraPos - worldPos);
	float3 lightDir = -normalize(directionalLit.direction.xyz);
	float3 halfDir = normalize(viewDir + lightDir);

	float ndotl = max(dot(normal, lightDir), 0.0f);
	float ndotv = max(dot(normal, viewDir), 0.0f);
	float vdoth = max(dot(viewDir, halfDir), 0.0f);

	float ndf = DistributionGGX(normal, halfDir, perceptualRoughness);
	float geometry = GeometrySmith(normal, viewDir, lightDir, perceptualRoughness);
	float3 fresnel = FresnelSchlick(vdoth, f0);
	float3 specular = (ndf * geometry * fresnel) / max(4.0f * ndotv * ndotl, 0.0001f);

	float3 diffuseColor = baseColor * (1.0f - metallic);
	float3 diffuse = (1.0f - fresnel) * diffuseColor / PI;
	float3 radiance = directionalLit.color.rgb;
	float3 color = (diffuse + specular) * radiance * ndotl * ao;
	color += 0.03f * baseColor * ao + emissive;
	color = pow(max(color, 0.0f), 1.0f / 2.2f);
	return float4(color, 1.0f);
}
