#version 450

#include "Lighting.h"

#define max_point_light_count 4

struct DirectionalLightRenderResource
{
    vec4 direction;
    vec4 color;
};

struct PointLightRenderResource
{
    mat4 viewMatrices[6];
    vec4 color;
    vec3 position;
    float padding;
};

layout(set = 0, binding = 0) uniform sampler2D gBufferA;
layout(set = 0, binding = 1) uniform sampler2D gBufferB;
layout(set = 0, binding = 2) uniform sampler2D gBufferC;
layout(set = 0, binding = 3) uniform sampler2D depthTex;
layout(set = 0, binding = 4) uniform sampler2D shadowMap0;
layout(set = 0, binding = 5) uniform sampler2D shadowMap1;
layout(set = 0, binding = 6) uniform sampler2D shadowMap2;
layout(set = 0, binding = 7) uniform sampler2D shadowMap3;
layout(set = 0, binding = 8) uniform sampler2D gtaoTex;

layout(std140, set = 1, binding = 0) uniform PerFrameCB
{
    mat4 viewMatrix;
    mat4 projMatrix;
    mat4 invViewProjection;
    vec3 camPos;
    float pointLightNumber;
    DirectionalLightRenderResource directionalLit;
    PointLightRenderResource pointLight[max_point_light_count];
} uboPerFrame;

layout(std140, set = 2, binding = 0) uniform CSMCB
{
    vec4 cascadeSplits;
    mat4 lightViewProj[4];
    vec4 renderOptions;
} uboCSM;

layout(location = 0) in vec2 inTexCoord;
layout(location = 0) out vec4 outColor;

float SampleShadowMap(int cascadeIndex, vec2 uv)
{
    if (cascadeIndex == 0) return texture(shadowMap0, uv).r;
    if (cascadeIndex == 1) return texture(shadowMap1, uv).r;
    if (cascadeIndex == 2) return texture(shadowMap2, uv).r;
    return texture(shadowMap3, uv).r;
}

vec3 ReconstructWorldPosition(vec2 uv, float depth)
{
    vec2 ndc = uv * 2.0 - 1.0;
    vec4 clip = vec4(ndc, depth, 1.0);
    vec4 world = uboPerFrame.invViewProjection * clip;
    return world.xyz / world.w;
}

float ComputeShadow(vec3 worldPos, vec3 normal, vec3 lightDir)
{
    float viewDepth = abs((uboPerFrame.viewMatrix * vec4(worldPos, 1.0)).z);
    int cascadeIndex = 0;
    if (viewDepth > uboCSM.cascadeSplits.x) cascadeIndex = 1;
    if (viewDepth > uboCSM.cascadeSplits.y) cascadeIndex = 2;
    if (viewDepth > uboCSM.cascadeSplits.z) cascadeIndex = 3;

    vec4 lightClip = uboCSM.lightViewProj[cascadeIndex] * vec4(worldPos, 1.0);
    vec3 shadowCoord = lightClip.xyz / lightClip.w;
    vec2 uv = shadowCoord.xy * 0.5 + 0.5;
    if (shadowCoord.z <= 0.0 || shadowCoord.z >= 1.0 || uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0)
    {
        return 1.0;
    }

    float bias = max(0.0008 * (1.0 - dot(normal, lightDir)), 0.0002);
    vec2 texelSize = 1.0 / vec2(textureSize(shadowMap0, 0));

    float lit = 0.0;
    for (int y = -1; y <= 1; ++y)
    {
        for (int x = -1; x <= 1; ++x)
        {
            float closestDepth = SampleShadowMap(cascadeIndex, uv + vec2(x, y) * texelSize);
            lit += (shadowCoord.z - bias) <= closestDepth ? 1.0 : 0.0;
        }
    }
    return lit / 9.0;
}

void main()
{
    vec4 gbufferAValue = texture(gBufferA, inTexCoord);
    vec4 gbufferBValue = texture(gBufferB, inTexCoord);
    vec4 gbufferCValue = texture(gBufferC, inTexCoord);
    float depth = texture(depthTex, inTexCoord).r;

    vec3 baseColor = gbufferAValue.rgb;
    float metallic = gbufferAValue.a;
    vec3 normal = normalize(gbufferBValue.rgb);
    float perceptualRoughness = max(gbufferBValue.a, 0.04);
    vec3 emissive = gbufferCValue.rgb;
    float ao = gbufferCValue.a;
    if (uboCSM.renderOptions.x > 0.5)
    {
        ao *= texture(gtaoTex, inTexCoord).r;
    }

    vec3 worldPos = ReconstructWorldPosition(inTexCoord, depth);

    vec3 f0 = vec3(0.04);
    vec3 specularColor = mix(f0, baseColor, metallic);
    vec3 v = normalize(uboPerFrame.camPos - worldPos);
    vec3 l = -normalize(uboPerFrame.directionalLit.direction.xyz);
    vec3 h = normalize(v + l);

    float nDotL = max(dot(normal, l), 0.0);
    float nDotV = max(dot(normal, v), 0.0);
    float vDotH = max(dot(v, h), 0.0);
    float ndf = DistributionGGX(normal, h, perceptualRoughness);
    float g = GeometrySmith(normal, v, l, perceptualRoughness);
    vec3 f = fresnelSchlick(vDotH, specularColor);
    vec3 specular = (ndf * g * f) / max(4.0 * nDotV * nDotL, 0.0001);
    vec3 diffuse = (vec3(1.0) - f) * baseColor * (1.0 - metallic) / PI;

    float shadow = ComputeShadow(worldPos, normal, l);
    vec3 radiance = uboPerFrame.directionalLit.color.rgb;
    vec3 color = (diffuse + specular) * radiance * nDotL * shadow * ao;
    color += vec3(0.03) * baseColor * ao + emissive;
    color = pow(color, vec3(1.0 / 2.2));
    outColor = vec4(color, 1.0);
}
