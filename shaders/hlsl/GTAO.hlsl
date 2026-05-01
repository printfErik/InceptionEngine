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

Texture2D NormalGBuffer : register(t0, space0);
Texture2D DepthGBuffer : register(t1, space0);
RWTexture2D<float> OutAO : register(u0, space0);
SamplerState LinearSampler : register(s0, space0);

static const float2 SampleDirections[8] =
{
	float2(1.0f, 0.0f),
	float2(-1.0f, 0.0f),
	float2(0.0f, 1.0f),
	float2(0.0f, -1.0f),
	float2(0.707f, 0.707f),
	float2(-0.707f, 0.707f),
	float2(0.707f, -0.707f),
	float2(-0.707f, -0.707f),
};

float3 ReconstructWorldPosition(float2 uv, float depth)
{
	float2 ndc = uv * 2.0f - 1.0f;
	ndc.y = -ndc.y;
	float4 clip = float4(ndc, depth, 1.0f);
	float4 world = mul(invViewProjMatrix, clip);
	return world.xyz / world.w;
}

float ViewDepth(float3 worldPos)
{
	return abs(mul(viewMatrix, float4(worldPos, 1.0f)).z);
}

[numthreads(16, 16, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	uint width;
	uint height;
	OutAO.GetDimensions(width, height);
	if (dispatchThreadId.x >= width || dispatchThreadId.y >= height)
	{
		return;
	}

	float2 uv = (float2(dispatchThreadId.xy) + 0.5f) / float2(width, height);
	float depth = DepthGBuffer.SampleLevel(LinearSampler, uv, 0).r;
	if (depth >= 0.99999f)
	{
		OutAO[dispatchThreadId.xy] = 1.0f;
		return;
	}

	float3 worldPos = ReconstructWorldPosition(uv, depth);
	float centerDepth = ViewDepth(worldPos);
	float3 normal = normalize(NormalGBuffer.SampleLevel(LinearSampler, uv, 0).xyz);

	float occlusion = 0.0f;
	float sampleCount = 0.0f;
	float2 texel = 1.0f / float2(width, height);
	for (uint ring = 1; ring <= 3; ++ring)
	{
		float radius = 2.0f + ring * 3.0f;
		for (uint dirIndex = 0; dirIndex < 8; ++dirIndex)
		{
			float2 sampleUv = saturate(uv + SampleDirections[dirIndex] * texel * radius);
			float sampleDepth = DepthGBuffer.SampleLevel(LinearSampler, sampleUv, 0).r;
			if (sampleDepth < 0.99999f)
			{
				float3 sampleWorld = ReconstructWorldPosition(sampleUv, sampleDepth);
				float3 toSample = sampleWorld - worldPos;
				float sampleViewDepth = ViewDepth(sampleWorld);
				float facing = saturate(dot(normal, normalize(toSample)) * 0.5f + 0.5f);
				float closeEnough = saturate(1.0f - abs(sampleViewDepth - centerDepth) / 3.0f);
				occlusion += (sampleViewDepth < centerDepth - 0.02f) ? facing * closeEnough : 0.0f;
			}
			sampleCount += 1.0f;
		}
	}

	float ao = saturate(1.0f - occlusion / max(sampleCount, 1.0f) * 1.8f);
	OutAO[dispatchThreadId.xy] = ao;
}
