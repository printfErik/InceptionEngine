#version 450

struct DirectionalLightRenderResource
{
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
}

struct PointLightRenderResource
{
    vec3 position;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	float constant;
	float linear;
	float quadratic;
}

layout(set = 1, binding = 1) uniform sampler2D DiffuseSampler;
layout(set = 1, binding = 2) uniform sampler2D SpecularSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCord;
layout(location = 2) in vec3 fragNormal;

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