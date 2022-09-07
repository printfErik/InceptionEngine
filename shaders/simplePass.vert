#version 450

layout(binding = 0) uniform uniformBufferObject
{
    mat4 model;
    mat4 view;
    mat4 projection;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCord;

void main()
{
    gl_Position = ubo.projection * ubo.view * ubo.model * vec4(inPosition, 1.0);
    fragColor = inColor;
    fragTexCord = inTexCord;
}