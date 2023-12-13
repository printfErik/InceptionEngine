#pragma once
#include "../../../core/icpMacros.h"

#include <vulkan/vulkan.hpp>

#include "vk_mem_alloc.h"

INCEPTION_BEGIN_NAMESPACE
	class icpVulkanUtility
{
public:
	static void CreateGPUBuffer(
		VkDeviceSize size,
		VkSharingMode sharingMode,
		VkBufferUsageFlags usage,
		VmaAllocator& allocator,
		VmaAllocation& allocation,
		VkBuffer& buffer,
		uint32_t queueFamilyIndexCount = 1,
		uint32_t* queueFamilyIndices = nullptr);

	static uint32_t findMemoryType(
		uint32_t typeFilter,
		VkMemoryPropertyFlags properties,
		VkPhysicalDevice& physicalDevice);

	static void CreateGPUImage(
		uint32_t width,
		uint32_t height,
		uint32_t mipmapLevel,
		uint32_t layerCount,
		VkFormat format,
		VkImageTiling tiling,
		VkImageUsageFlags usage,
		VmaAllocator& allocator,
		VkImage& image,
		VmaAllocation& allocation
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

	static VkImageView CreateGPUImageView(
		VkImage image,
		VkImageViewType viewType,
		VkFormat format,
		VkImageAspectFlags aspectFlags,
		uint32_t mipmapLevel,
		uint32_t layerCount,
		VkDevice& device
	);

	static VkFormat findSupportedFormat(
		const std::vector<VkFormat>& candidates,
		VkImageTiling tiling,
		VkFormatFeatureFlags features,
		VkPhysicalDevice& physicalDevice
	);

	static VkFormat findDepthFormat(VkPhysicalDevice physicalDevice);

	static VkShaderModule createShaderModule(
		const char* filePath,
		VkDevice device
	);

	static void copyBuffer(
		VkBuffer srcBuffer,
		VkBuffer dstBuffer,
		VkDeviceSize size,
		VkDevice device,
		VkCommandPool cbp,
		VkQueue queue
	);

	static void transitionImageLayout(
		VkImage image, 
		VkFormat format, 
		VkImageLayout oldLayout, 
		VkImageLayout newLayout, 
		uint32_t mipmapLevel,
		VkCommandPool cbp, 
		VkDevice device, 
		VkQueue queue
	);

	static void copyBuffer2Image(
		VkBuffer srcBuffer,
		VkImage dstImage,
		uint32_t width,
		uint32_t height,
		VkCommandPool cbp,
		VkDevice device,
		VkQueue queue
	);

	static void generateMipmaps(
		VkImage image, 
		VkFormat imageFormat, 
		int32_t width, 
		int32_t height, 
		uint32_t mipmapLevels,
		VkCommandPool cbp,
		VkDevice device,
		VkQueue queue,
		VkPhysicalDevice physicalDevice
	);
};


INCEPTION_END_NAMESPACE