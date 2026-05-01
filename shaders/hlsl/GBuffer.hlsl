#pragma pack_matrix(column_major)

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

cbuffer UBOMeshRenderResource : register(b0, space0)
{
	float4x4 modelMatrix;
	float4x4 normalMatrix;
};

cbuffer UBOPerMaterial : register(b0, space1)
{
	float4 baseColorFactor;
	float4 emissiveFactor;
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

Texture2D BaseColorSampler : register(t0, space1);
Texture2D MetallicRoughnessSampler : register(t1, space1);
Texture2D MetallicSampler : register(t2, space1);
Texture2D RoughnessSampler : register(t3, space1);
Texture2D NormalSampler : register(t4, space1);
Texture2D AoSampler : register(t5, space1);
Texture2D EmissiveSampler : register(t6, space1);
SamplerState LinearSampler : register(s0, space0);

struct VSInput
{
	float3 position : POSITION;
	float3 color : COLOR;
	float3 normal : NORMAL;
	float2 texCoord : TEXCOORD;
};

struct VSOutput
{
	float4 position : SV_POSITION;
	float3 color : COLOR0;
	float2 texCoord : TEXCOORD0;
	float3 normal : NORMAL0;
	float3 worldPos : TEXCOORD1;
};

VSOutput VSMain(VSInput input)
{
	VSOutput output;
	float4 world = mul(modelMatrix, float4(input.position, 1.0f));
	output.worldPos = world.xyz;
	output.normal = normalize(mul((float3x3)normalMatrix, input.normal));
	output.color = input.color;
	output.texCoord = input.texCoord;
	output.position = mul(projMatrix, mul(viewMatrix, world));
	return output;
}

float3 GetNormalFromMap(VSOutput input)
{
	float3 tangentNormal = NormalSampler.Sample(LinearSampler, input.texCoord).xyz * 2.0f - 1.0f;
	float3 q1 = ddx(input.worldPos);
	float3 q2 = ddy(input.worldPos);
	float2 st1 = ddx(input.texCoord);
	float2 st2 = ddy(input.texCoord);
	float3 n = normalize(input.normal);
	float3 t = normalize(q1 * st2.y - q2 * st1.y);
	float3 b = normalize(q2 * st1.x - q1 * st2.x);
	float3x3 tbn = float3x3(t, b, n);
	return normalize(mul(tangentNormal, tbn));
}

struct PSOutput
{
	float4 gbufferA : SV_Target0;
	float4 gbufferB : SV_Target1;
	float4 gbufferC : SV_Target2;
};

PSOutput PSMain(VSOutput input)
{
	PSOutput output;
	float3 baseColor = colorTextureSet > -1.0f
		? pow(BaseColorSampler.Sample(LinearSampler, input.texCoord).rgb, 2.2f) * baseColorFactor.rgb
		: baseColorFactor.rgb;

	float metallic = PhysicalDescriptorTextureSet > -1.0f
		? MetallicRoughnessSampler.Sample(LinearSampler, input.texCoord).g * metallicFactor
		: metallicTextureSet > -1.0f
			? MetallicSampler.Sample(LinearSampler, input.texCoord).r * metallicFactor
			: metallicFactor;

	float roughness = PhysicalDescriptorTextureSet > -1.0f
		? MetallicRoughnessSampler.Sample(LinearSampler, input.texCoord).b * roughnessFactor
		: roughnessTextureSet > -1.0f
			? RoughnessSampler.Sample(LinearSampler, input.texCoord).r * roughnessFactor
			: roughnessFactor;

	float3 normal = normalTextureSet > -1.0f ? GetNormalFromMap(input) : normalize(input.normal);
	float ao = occlusionTextureSet > -1.0f ? AoSampler.Sample(LinearSampler, input.texCoord).r : 1.0f;
	float3 emissive = emissiveTextureSet > -1.0f
		? pow(EmissiveSampler.Sample(LinearSampler, input.texCoord).rgb, 2.2f) * emissiveFactor.rgb
		: emissiveFactor.rgb;

	output.gbufferA = float4(baseColor, metallic);
	output.gbufferB = float4(normal, roughness);
	output.gbufferC = float4(emissive, ao);
	return output;
}
