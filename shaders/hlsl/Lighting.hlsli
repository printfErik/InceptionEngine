#pragma once

static const float PI = 3.14159265359f;

float DistributionGGX(float3 N, float3 H, float roughness)
{
	float a2 = roughness * roughness;
	float roughnessSq = a2 * a2;
	float NdotH = max(dot(N, H), 0.0f);
	float NdotH2 = NdotH * NdotH;
	float denom = (NdotH2 * (roughnessSq - 1.0f) + 1.0f);
	return roughnessSq / max(PI * denom * denom, 0.0001f);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
	float r = roughness + 1.0f;
	float k = (r * r) / 8.0f;
	return NdotV / max(NdotV * (1.0f - k) + k, 0.0001f);
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
	float NdotV = max(dot(N, V), 0.0f);
	float NdotL = max(dot(N, L), 0.0f);
	return GeometrySchlickGGX(NdotV, roughness) * GeometrySchlickGGX(NdotL, roughness);
}

float3 FresnelSchlick(float cosTheta, float3 F0)
{
	return F0 + (1.0f - F0) * pow(saturate(1.0f - cosTheta), 5.0f);
}
