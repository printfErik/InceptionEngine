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

	glm::vec3 m_ambient{};
	glm::vec3 m_diffuse{};
	glm::vec3 m_specular{};
};

class icpDirectionalLightComponent : public icpLightComponent
{
public:
	glm::vec3 m_direction{};
};

class icpPointlLightComponent : public icpLightComponent
{
public:
	glm::vec3 m_posiotion{};

	float constant = 0.f;
	float linear = 0.f;
	float quadratic = 0.f;
};

INCEPTION_END_NAMESPACE