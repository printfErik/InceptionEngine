#pragma once

#include "../core/icpMacros.h"
#include "../core/icpGuid.h"


#include "../scene/icpComponent.h"
#include <vulkan/vulkan.hpp>

INCEPTION_BEGIN_NAMESPACE

enum class eLightType
{
	DIRECTIONAL_LIGHT,
	POINT_LIGHT,
	SPOT_LIGHT,
};


class icpLightComponent : public icpComponentBase
{
public:

	icpLightComponent() = default;
	virtual ~icpLightComponent() = default;

	
};

INCEPTION_END_NAMESPACE