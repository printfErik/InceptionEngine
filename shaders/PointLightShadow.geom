#version 450

#define MAX_POINT_LIGHT_COUNT 4
layout (triangles, invocations = MAX_POINT_LIGHT_COUNT * 6) in;

struct DirectionalLightRenderResource
{
    vec4 direction;
    vec4 color;
};

struct PointLightRenderResource
{
    vec3 position;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	float constant;
	float linear;
	float quadratic;
};

layout(set = 2, binding = 0) uniform PerFrameCB
{
    mat4 viewMatrix;
    mat4 projMatrix;
    vec3 cameraPos;
    float pointLightNumber;
    DirectionalLightRenderResource directionalLit;
    PointLightRenderResource pointLight[max_point_light_count];
} uboPerFrame;

void main()
{
    gl_Layer = gl_InvocationID;
}