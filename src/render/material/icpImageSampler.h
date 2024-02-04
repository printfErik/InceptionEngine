#pragma once

#include "../../core/icpMacros.h"
#include <vulkan/vulkan.hpp>
INCEPTION_BEGIN_NAMESPACE

class icpSamplerBuilder
{
public:
	icpSamplerBuilder();
	virtual ~icpSamplerBuilder();

	VkSampler BuildSampler();

private:
};


INCEPTION_END_NAMESPACE