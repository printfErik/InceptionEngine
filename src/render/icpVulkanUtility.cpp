#include "icpVulkanUtility.h"

INCEPTION_BEGIN_NAMESPACE

uint32_t icpVulkanUtility::findMemoryType(
	uint32_t typeFilter, 
	VkMemoryPropertyFlags properties,
	VkPhysicalDevice& physicalDevice)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
	{
		if ((typeFilter & (1 << i)) && (properties & memProperties.memoryTypes[i].propertyFlags) == properties)
		{
			return i;
		}
	}
	throw std::runtime_error("failed to find memory type");
	return UINT32_MAX;
}

void icpVulkanUtility::createVulkanBuffer(
	VkDeviceSize size,
	VkSharingMode sharingMode,
	VkBufferUsageFlags usage,
	VkMemoryPropertyFlags properties,
	VkBuffer& buffer,
	VkDeviceMemory& bufferMemory,
	VkDevice& device,
	VkPhysicalDevice& physicalDevice)
{
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.sharingMode = sharingMode;
	bufferInfo.usage = usage;

	if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create vertex buffer!");
	}

	VkMemoryRequirements memRequirement{};

	vkGetBufferMemoryRequirements(device, buffer, &memRequirement);

	VkMemoryAllocateInfo allocateInfo{};
	allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocateInfo.allocationSize = memRequirement.size;
	allocateInfo.memoryTypeIndex = findMemoryType(memRequirement.memoryTypeBits, VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, physicalDevice);

	if (vkAllocateMemory(device, &allocateInfo, nullptr, &bufferMemory) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate memory!");
	}

	vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

void icpVulkanUtility::createVulkanImage(
	uint32_t width,
	uint32_t height,
	VkFormat format,
	VkImageTiling tiling,
	VkImageUsageFlags usage,
	VkMemoryPropertyFlags properties,
	VkImage& image,
	VkDeviceMemory& imageMem,
	VkDevice& device,
	VkPhysicalDevice& physicalDevice
)
{
	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

	if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create image");
	}

	VkMemoryRequirements memRequirement{};

	vkGetImageMemoryRequirements(device, image, &memRequirement);

	VkMemoryAllocateInfo allocateInfo{};
	allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocateInfo.allocationSize = memRequirement.size;
	allocateInfo.memoryTypeIndex = icpVulkanUtility::findMemoryType(
		memRequirement.memoryTypeBits,
		properties, physicalDevice);

	if (vkAllocateMemory(device, &allocateInfo, nullptr, &imageMem) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate memory!");
	}

	vkBindImageMemory(device, image, imageMem, 0);
}

VkCommandBuffer icpVulkanUtility::beginSingleTimeCommands(VkCommandPool& cbPool, VkDevice& device)
{
	VkCommandBuffer commandBuffer;

	VkCommandBufferAllocateInfo gAllocInfo{};
	gAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	gAllocInfo.commandPool = cbPool;
	gAllocInfo.commandBufferCount = 1;
	gAllocInfo.level = VkCommandBufferLevel::VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	if (vkAllocateCommandBuffers(device, &gAllocInfo, &commandBuffer) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate command buffer!");
	}

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("failed to begin recording command buffer!");
	}

	return commandBuffer;
}

void icpVulkanUtility::endSingleTimeCommandsAndSubmit(
	VkCommandBuffer cb,
	VkQueue& queue,
	VkCommandPool& cbPool,
	VkDevice& device)
{
	if (vkEndCommandBuffer(cb) != VK_SUCCESS) {
		throw std::runtime_error("failed to record command buffer!");
	}

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cb;

	if (vkQueueSubmit(queue, 1, &submitInfo, nullptr) != VK_SUCCESS) {
		throw std::runtime_error("failed to submit draw command buffer!");
	}

	vkQueueWaitIdle(queue);
	vkFreeCommandBuffers(device, cbPool, 1, &cb);
}



INCEPTION_END_NAMESPACE