
#define MAX_SPOT_LIGHT_COUNT 4
#define MAX_POINT_LIGHT_COUNT 4

struct DirectionalLightRenderResource
{
    vec4 direction;
    vec4 ambient;
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

struct SpotLightRenderResource
{
	mat4 VPMatrix;
	vec4 color;
	vec3 position;
	float innerConeAngle;
	vec3 direction;
	float outerConeAngle;
};