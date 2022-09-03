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


INCEPTION_END_NAMESPACE