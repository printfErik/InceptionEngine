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

	static void createVulkanImage(
		uint32_t width,
		uint32_t height,
		uint32_t mipmapLevel,
		VkFormat format,
		VkImageTiling tiling,
		VkImageUsageFlags usage,
		VkMemoryPropertyFlags properties,
		VkImage& image,
		VkDeviceMemory& imageMem,
		VkDevice& device,
		VkPhysicalDevice& physicalDevice
	);

	static VkCommandBuffer beginSingleTimeCommands(
		VkCommandPool& cbPool,
		VkDevice& device
	);

	static void endSingleTimeCommandsAndSubmit(
		VkCommandBuffer cb,
		VkQueue& queue,
		VkCommandPool& cbPool,
		VkDevice& device
	);

	static VkImageView createImageView(
		VkImage image,
		VkFormat format,
		VkImageAspectFlags aspectFlags,
		uint32_t mipmapLevel,
		VkDevice& device
	);

	static VkFormat findSupportedFormat(
		const std::vector<VkFormat>& candidates,
		VkImageTiling tiling,
		VkFormatFeatureFlags features,
		VkPhysicalDevice& physicalDevice
	);

	static VkFormat findDepthFormat(VkPhysicalDevice physicalDevice);
};


INCEPTION_END_NAMESPACE