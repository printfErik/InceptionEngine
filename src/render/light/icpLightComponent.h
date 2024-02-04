#pragma once

#include "../../core/icpMacros.h"
#include "../../core/icpGuid.h"

#include "../../scene/icpComponent.h"
#include <vulkan/vulkan.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

INCEPTION_BEGIN_NAMESPACE

enum class eLightType
{
	DIRECTIONAL_LIGHT,
	POINT_LIGHT,
	SPOT_LIGHT,
	NULL_LIGHT
};

class icpLightComponent : public icpComponentBase
{
public:

	icpLightComponent() = default;
	virtual ~icpLightComponent() = default;

	eLightType m_type = eLightType::NULL_LIGHT;

	glm::vec3 m_color{};
};

class icpDirectionalLightComponent : public icpLightComponent
{
public:
	glm::vec3 m_direction{};
};

class icpPointLightComponent : public icpLightComponent
{
public:
	glm::vec3 m_position{};
	float constant = 0.f;
	float linear = 0.f;
	float quadratic = 0.f;
};

class icpSpotLightComponent : public icpLightComponent
{
public:
	glm::vec3 m_position{};
	glm::vec3 m_direction{};

	float m_innerConeAngle = 0.f;
	float m_outerConeAngle = 0.f;
};

INCEPTION_END_NAMESPACE