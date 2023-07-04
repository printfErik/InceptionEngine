#include "icpVulkanUtility.h"
#include <fstream>
#include <iterator>

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

void icpVulkanUtility::CreateGPUBuffer(
	VkDeviceSize size,
	VkSharingMode sharingMode,
	VkBufferUsageFlags usage,
	VmaAllocator& allocator,
	VmaAllocation& allocation,
	VkBuffer& buffer)
{
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.sharingMode = sharingMode;
	bufferInfo.usage = usage;


	/*
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
	*/

	VmaAllocationCreateInfo vma_create_info{};
	vma_create_info.usage = VMA_MEMORY_USAGE_AUTO;
	vma_create_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;

	vmaCreateBuffer(allocator, &bufferInfo, &vma_create_info, &buffer, &allocation, nullptr);
}

void icpVulkanUtility::CreateGPUImage(
	uint32_t width,
	uint32_t height,
	uint32_t mipmapLevel,
	VkFormat format,
	VkImageTiling tiling,
	VkImageUsageFlags usage,
	VmaAllocator& allocator,
	VkImage& image,
	VmaAllocation& allocation
)
{
	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = mipmapLevel;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;


	/*
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
	*/

	VmaAllocationCreateInfo memory_info{};
	memory_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

	vmaCreateImage(allocator, &imageInfo, &memory_info, &image, &allocation, nullptr);
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

VkImageView icpVulkanUtility::CreateGPUImageView(
	VkImage image, 
	VkFormat format,
	VkImageAspectFlags aspectFlags,
	uint32_t mipmapLevel,
	VkDevice& device)
{
	VkImageViewCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	createInfo.image = image;
	createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	createInfo.format = format;
	createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.subresourceRange.aspectMask = aspectFlags;
	createInfo.subresourceRange.baseMipLevel = 0;
	createInfo.subresourceRange.levelCount = mipmapLevel;
	createInfo.subresourceRange.baseArrayLayer = 0;
	createInfo.subresourceRange.layerCount = 1;

	VkImageView view;
	if (vkCreateImageView(device, &createInfo, nullptr, &view) != VK_SUCCESS) {
		throw std::runtime_error("failed to create image views!");
	}
	return view;
}

VkFormat icpVulkanUtility::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features, VkPhysicalDevice& physicalDevice)
{
	for (VkFormat format : candidates) 
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
			return format;
		}
	}

	throw std::runtime_error("failed to find supported format!");
}

VkFormat icpVulkanUtility::findDepthFormat(VkPhysicalDevice physicalDevice) {
	return icpVulkanUtility::findSupportedFormat(
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT,
		physicalDevice
	);
}

VkShaderModule icpVulkanUtility::createShaderModule(const char* shaderFileName, VkDevice device)
{
	std::ifstream inFile(shaderFileName, std::ios::binary | std::ios::ate);
	size_t fileSize = (size_t)inFile.tellg();
	std::vector<char> content(fileSize);

	inFile.seekg(0);
	inFile.read(content.data(), fileSize);

	inFile.close();

	VkShaderModuleCreateInfo creatInfo{};
	creatInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	creatInfo.pCode = reinterpret_cast<const uint32_t*>(content.data());
	creatInfo.codeSize = content.size();

	VkShaderModule shader{ VK_NULL_HANDLE };
	if (vkCreateShaderModule(device, &creatInfo, nullptr, &shader) != VK_SUCCESS)
	{
		throw std::runtime_error("create shader module failed");
	}

	return shader;
}

void icpVulkanUtility::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkDevice device, VkCommandPool cbp, VkQueue queue)
{
	VkCommandBuffer commandBuffer = icpVulkanUtility::beginSingleTimeCommands(cbp, device);

	VkBufferCopy copyRegin{};
	copyRegin.srcOffset = 0;
	copyRegin.dstOffset = 0;
	copyRegin.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegin);

	icpVulkanUtility::endSingleTimeCommandsAndSubmit(
		commandBuffer,
		queue,
		cbp,
		device
	);
}

void icpVulkanUtility::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipmapLevel, VkCommandPool cbp, VkDevice device, VkQueue queue)
{

	VkCommandBuffer commandBuffer = icpVulkanUtility::beginSingleTimeCommands(cbp, device);

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;

	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = mipmapLevel;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else {
		throw std::invalid_argument("unsupported layout transition!");
	}

	vkCmdPipelineBarrier(
		commandBuffer,
		sourceStage, destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	icpVulkanUtility::endSingleTimeCommandsAndSubmit(commandBuffer, queue, cbp, device);
}


void icpVulkanUtility::copyBuffer2Image(VkBuffer srcBuffer, VkImage dstImage, uint32_t width, uint32_t height, VkCommandPool cbp, VkDevice device, VkQueue queue)
{
	VkCommandBuffer commandBuffer = icpVulkanUtility::beginSingleTimeCommands(cbp, device);

	VkBufferImageCopy copyRegin{};
	copyRegin.bufferOffset = 0;
	copyRegin.bufferImageHeight = 0;
	copyRegin.bufferRowLength = 0;
	copyRegin.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copyRegin.imageSubresource.mipLevel = 0;
	copyRegin.imageSubresource.layerCount = 1;
	copyRegin.imageSubresource.baseArrayLayer = 0;
	copyRegin.imageOffset = { 0,0,0 };
	copyRegin.imageExtent = { width, height, 1 };

	vkCmdCopyBufferToImage(commandBuffer, srcBuffer, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegin);

	icpVulkanUtility::endSingleTimeCommandsAndSubmit(
		commandBuffer,
		queue,
		cbp,
		device
	);
}

void icpVulkanUtility::generateMipmaps(VkImage image, VkFormat imageFormat, int32_t width, int32_t height, uint32_t mipmapLevels, VkCommandPool cbp, VkDevice device, VkQueue queue, VkPhysicalDevice physicalDevice)
{
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(physicalDevice, imageFormat,
		&formatProperties);

	if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
	{
		throw std::runtime_error("texture image format does not support linear blitting!");
	}

	VkCommandBuffer cb = icpVulkanUtility::beginSingleTimeCommands(cbp, device);

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = image;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.layerCount = 1;

	int32_t mipmapWidth = width;
	int32_t mipmapHeight = height;

	for (uint32_t i = 1; i < mipmapLevels; i++)
	{
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		vkCmdPipelineBarrier(cb,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		VkImageBlit blit{};
		blit.srcOffsets[0] = { 0, 0, 0 };
		blit.srcOffsets[1] = { mipmapWidth, mipmapHeight, 1 };
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = i - 1;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;
		blit.dstOffsets[0] = { 0, 0, 0 };
		blit.dstOffsets[1] = { mipmapWidth > 1 ? mipmapWidth / 2 : 1, mipmapHeight > 1 ? mipmapHeight / 2 : 1, 1 };
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = i;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;

		vkCmdBlitImage(cb,
			image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &blit,
			VK_FILTER_LINEAR);

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(cb,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		if (mipmapWidth > 1) mipmapWidth /= 2;
		if (mipmapHeight > 1) mipmapHeight /= 2;
	}

	barrier.subresourceRange.baseMipLevel = mipmapLevels - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(cb,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
		0, nullptr,
		0, nullptr,
		1, &barrier);

	icpVulkanUtility::endSingleTimeCommandsAndSubmit(cb, queue, cbp, device);

}




INCEPTION_END_NAMESPACE