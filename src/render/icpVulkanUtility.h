#pragma once
#include "../core/icpMacros.h"

#include <vulkan/vulkan.hpp>

INCEPTION_BEGIN_NAMESPACE

class icpVulkanUtility
{
public:
	static void createVulkanBuffer(
		VkDeviceSize size,
		VkSharingMode sharingMode,
		VkBufferUsageFlags usage, 
		VkMemoryPropertyFlags properties, 
		VkBuffer& buffer, 
		VkDeviceMemory& bufferMemory,
		VkDevice& device,
		VkPhysicalDevice& physicalDevice);

	static uint32_t findMemoryType(
		uint32_t typeFilter,
		VkMemoryPropertyFlags properties,
		VkPhysicalDevice& physicalDevice);
};


INCEPTION_END_NAMESPACE