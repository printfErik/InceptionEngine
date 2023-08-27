#pragma once

#include "../../core/icpMacros.h"
#include <vulkan/vulkan.h>

INCEPTION_BEGIN_NAMESPACE

struct icpBufferRenderResourceInfo
{
	VkBuffer buffer;
	uint64_t offset;
	uint64_t range;
};

INCEPTION_END_NAMESPACE