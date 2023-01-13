#version 450

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCord;

layout(location = 0) out vec4 outColor;

void main() {

    vec2 size = textureSize(texSampler, 0);
    if (size.x == 1.0 && size.y == 1.0)
    {
        outColor = vec4(fragColor, 1);
    }
    else
    {
        outColor = texture(texSampler, fragTexCord);
    }        
}