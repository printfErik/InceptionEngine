#pragma once

#include "../../core/icpMacros.h"
#include <vulkan/vulkan.h>

#include "vk_mem_alloc.h"

INCEPTION_BEGIN_NAMESPACE

struct icpBufferRenderResource
{
	VkBuffer buffer{ VK_NULL_HANDLE };
	VmaAllocation bufferAllocation{ VK_NULL_HANDLE };
	uint64_t range = 0;
	uint64_t offset = 0;
};

INCEPTION_END_NAMESPACE