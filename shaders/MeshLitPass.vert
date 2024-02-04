#version 450
#include "PerFrameGlobalUBO.h"

layout(set = 0, binding = 0) uniform UBOMeshRenderResource
{
    mat4 modelMatrix;
    mat4 normalMatrix;
} uboPerMesh;

layout(set = 2, binding = 0) uniform PerFrameCB
{
    mat4 viewMatrix;
    mat4 projMatrix;
    vec3 cameraPos;
    float pointLightNumber;
    float spotLightNumber;
    DirectionalLightRenderResource directionalLit;
    PointLightRenderResource pointLightList[max_point_light_count];
    SpotLightRenderResource spotLightList[max_spot_light_count];
} uboPerFrame;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec3 worldPos;

void main()
{
    fragColor = inColor;
    fragTexCoord = inTexCoord;
    fragNormal = normalize(mat3(uboPerMesh.normalMatrix) * inNormal);
    vec4 worldPosV4 = uboPerMesh.modelMatrix * vec4(inPosition, 1.0);
    worldPos = worldPosV4.rgb;
    gl_Position = uboPerFrame.projMatrix *  uboPerFrame.viewMatrix * worldPosV4;
}