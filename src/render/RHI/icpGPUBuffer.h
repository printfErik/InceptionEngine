#pragma once

#include "../../core/icpMacros.h"
#include <vulkan/vulkan.h>

#include "vk_mem_alloc.h"

INCEPTION_BEGIN_NAMESPACE

struct icpBufferRenderResource
{
	VkBuffer buffer{ VK_NULL_HANDLE };
	VmaAllocation bufferAllocation{ VK_NULL_HANDLE };
};

class icpBufferWriteDS
{
public:
	icpBufferWriteDS() = delete;
	icpBufferWriteDS(const icpBufferRenderResource& bufferRes,
		VkDescriptorType type,
		uint32_t dstBinding,
		uint64_t _range,
		uint64_t _offset = 0
	);

	uint64_t offset = 0;
	uint64_t range = 0;
	VkDescriptorBufferInfo bufferInfo{};
	VkWriteDescriptorSet bufferWriteDS{};
	
};

INCEPTION_END_NAMESPACE