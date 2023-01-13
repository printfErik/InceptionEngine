#version 450

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCord;

layout(location = 0) out vec4 outColor;

void main() {

    if (fragTexCord.x >= 0.0 && fragTexCord.y >= 0.0)
    {
        outColor = texture(texSampler, fragTexCord);
    }
    else{
        outColor = fragColor;
    }
}