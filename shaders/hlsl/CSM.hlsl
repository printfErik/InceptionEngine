#pragma pack_matrix(column_major)

cbuffer UBOMeshRenderResource : register(b0, space0)
{
	float4x4 modelMatrix;
	float4x4 normalMatrix;
};

cbuffer CSMCB : register(b0, space3)
{
	float4 cascadeSplits;
	float4x4 lightViewProj[4];
};

cbuffer CSMRootConstants : register(b1, space3)
{
	uint cascadeIndex;
};

struct VSInput
{
	float3 position : POSITION;
	float3 color : COLOR;
	float3 normal : NORMAL;
	float2 texCoord : TEXCOORD;
};

float4 VSMain(VSInput input) : SV_POSITION
{
	float4 world = mul(modelMatrix, float4(input.position, 1.0f));
	return mul(lightViewProj[cascadeIndex], world);
}

void PSMain()
{
}
