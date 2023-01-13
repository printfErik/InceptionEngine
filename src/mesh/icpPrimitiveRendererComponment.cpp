#include "icpPrimitiveRendererComponment.h"
#include "../core/icpSystemContainer.h"
#include "../core/icpLogSystem.h"
#include "../render/icpRenderSystem.h"
#include "../render/icpVulkanUtility.h"
#include "../mesh/icpMeshData.h"

INCEPTION_BEGIN_NAMESPACE


void icpPrimitiveRendererComponment::fillInPrimitiveData(const glm::vec3& color)
{
	switch (m_primitive)
	{
	case ePrimitiveType::CUBE:
	{
		std::vector<icpVertex> cubeVertices {
		{{1,1,1}, color ,{-1,-1}},
		{{-1,1,1},color,{-1,-1}},
		{{-1,1,-1},color,{-1,-1}},
		{{1,1,-1},color,{-1,-1}},
		{{1,-1,1},color,{-1,-1}},
		{{-1,-1,1},color,{-1,-1}},
		{{-1,-1,-1},color,{-1,-1}},
		{{1,-1,-1},color,{-1,-1}},
		};

		m_vertices.assign(cubeVertices.begin(), cubeVertices.end());

		std::vector<uint32_t> cubeIndex{
			0, 3, 1, 3, 2, 1, 4, 5, 7, 5, 6, 7, 0, 4, 7, 0, 7, 3, 1, 2, 6, 1, 6, 5, 1, 4, 0, 1, 5, 4, 3, 7, 6, 3, 6, 2
		};

		m_vertexIndices.assign(cubeIndex.begin(), cubeIndex.end());
	}
	break;
	default:
	{
		ICP_LOG_WARING("no such primitive");
	}
	break;
	}
}

void icpPrimitiveRendererComponment::createVertexBuffers()
{
	auto vulkanRHI = dynamic_pointer_cast<icpVulkanRHI>(g_system_container.m_renderSystem->m_rhi);

	auto bufferSize = sizeof(m_vertices[0]) * m_vertices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMem;

	VkSharingMode mode = vulkanRHI->m_queueIndices.m_graphicsFamily.value() == vulkanRHI->m_queueIndices.m_transferFamily.value() ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;

	icpVulkanUtility::createVulkanBuffer(bufferSize,
		mode,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer,
		stagingBufferMem,
		vulkanRHI->m_device,
		vulkanRHI->m_physicalDevice);

	void* data;
	vkMapMemory(vulkanRHI->m_device, stagingBufferMem, 0, bufferSize, 0, &data);
	memcpy(data, m_vertices.data(), (size_t)bufferSize);
	vkUnmapMemory(vulkanRHI->m_device, stagingBufferMem);

	icpVulkanUtility::createVulkanBuffer(bufferSize,
		mode,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_vertexBuffer,
		m_vertexBufferMem,
		vulkanRHI->m_device,
		vulkanRHI->m_physicalDevice
	);

	icpVulkanUtility::copyBuffer(stagingBuffer,
		m_vertexBuffer,
		bufferSize,
		vulkanRHI->m_device,
		vulkanRHI->m_transferCommandPool,
		vulkanRHI->m_transferQueue
	);

	vkDestroyBuffer(vulkanRHI->m_device, stagingBuffer, nullptr);
	vkFreeMemory(vulkanRHI->m_device, stagingBufferMem, nullptr);
}

void icpPrimitiveRendererComponment::createIndexBuffers()
{
	auto vulkanRHI = dynamic_pointer_cast<icpVulkanRHI>(g_system_container.m_renderSystem->m_rhi);


	VkDeviceSize bufferSize = sizeof(m_vertexIndices[0]) * m_vertexIndices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMem;

	VkSharingMode mode = vulkanRHI->m_queueIndices.m_graphicsFamily.value() == vulkanRHI->m_queueIndices.m_transferFamily.value() ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;

	icpVulkanUtility::createVulkanBuffer(
		bufferSize,
		mode,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD,
		stagingBuffer,
		stagingBufferMem,
		vulkanRHI->m_device,
		vulkanRHI->m_physicalDevice);

	void* data;
	vkMapMemory(vulkanRHI->m_device, stagingBufferMem, 0, bufferSize, 0, &data);
	memcpy(data, m_vertexIndices.data(), (size_t)bufferSize);
	vkUnmapMemory(vulkanRHI->m_device, stagingBufferMem);

	icpVulkanUtility::createVulkanBuffer(bufferSize,
		mode,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_indexBuffer,
		m_indexBufferMem,
		vulkanRHI->m_device,
		vulkanRHI->m_physicalDevice);

	icpVulkanUtility::copyBuffer(stagingBuffer,
		m_indexBuffer,
		bufferSize,
		vulkanRHI->m_device,
		vulkanRHI->m_transferCommandPool,
		vulkanRHI->m_transferQueue
	);

	vkDestroyBuffer(vulkanRHI->m_device, stagingBuffer, nullptr);
	vkFreeMemory(vulkanRHI->m_device, stagingBufferMem, nullptr);
}

void icpPrimitiveRendererComponment::allocateDescriptorSets()
{
	auto vulkanRHI = dynamic_pointer_cast<icpVulkanRHI>(g_system_container.m_renderSystem->m_rhi);
	std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, vulkanRHI->m_meshDSLayout);

	VkDescriptorSetAllocateInfo allocateInfo{};
	allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocateInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
	allocateInfo.descriptorPool = vulkanRHI->m_descriptorPool;
	allocateInfo.pSetLayouts = layouts.data();

	m_descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

	if (vkAllocateDescriptorSets(vulkanRHI->m_device, &allocateInfo, m_descriptorSets.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate descriptor sets!");
	}

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = m_uniformBuffers[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);

		VkDescriptorImageInfo imageInfo{};

		imageInfo.imageView = m_textureImageView;
		imageInfo.sampler = m_textureSampler;
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = m_descriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = m_descriptorSets[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pImageInfo = &imageInfo;

		vkUpdateDescriptorSets(vulkanRHI->m_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}

void icpPrimitiveRendererComponment::createUniformBuffers()
{
	auto vulkanRHI = dynamic_pointer_cast<icpVulkanRHI>(g_system_container.m_renderSystem->m_rhi);
	auto bufferSize = sizeof(UniformBufferObject);

	m_uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	m_uniformBufferMem.resize(MAX_FRAMES_IN_FLIGHT);

	VkSharingMode mode = vulkanRHI->m_queueIndices.m_graphicsFamily.value() == vulkanRHI->m_queueIndices.m_transferFamily.value() ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		icpVulkanUtility::createVulkanBuffer(
			bufferSize,
			mode,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD,
			m_uniformBuffers[i],
			m_uniformBufferMem[i],
			vulkanRHI->m_device,
			vulkanRHI->m_physicalDevice);
	}
}

void icpPrimitiveRendererComponment::createTextureImages()
{
	auto vulkanRHI = dynamic_pointer_cast<icpVulkanRHI>(g_system_container.m_renderSystem->m_rhi);

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMem;

	std::vector<char> emptyImgData{0, 0, 0, 0};

	icpVulkanUtility::createVulkanBuffer(
		4, //1 * 1 * 4 rgba
		VK_SHARING_MODE_EXCLUSIVE,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer,
		stagingBufferMem,
		vulkanRHI->m_device,
		vulkanRHI->m_physicalDevice
	);

	void* data;
	vkMapMemory(vulkanRHI->m_device, stagingBufferMem, 0, static_cast<uint32_t>(4), 0, &data);
	memcpy(data, emptyImgData.data(), 4);
	vkUnmapMemory(vulkanRHI->m_device, stagingBufferMem);

	icpVulkanUtility::createVulkanImage(
		static_cast<uint32_t>(1),
		static_cast<uint32_t>(1),
		static_cast<uint32_t>(1),
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_textureImage,
		m_textureBufferMem,
		vulkanRHI->m_device,
		vulkanRHI->m_physicalDevice
	);

	transitionImageLayout(m_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<uint32_t>(1));
	copyBuffer2Image(stagingBuffer, m_textureImage, static_cast<uint32_t>(1), static_cast<uint32_t>(1));
	transitionImageLayout(m_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);

	vkDestroyBuffer(vulkanRHI->m_device, stagingBuffer, nullptr);
	vkFreeMemory(vulkanRHI->m_device, stagingBufferMem, nullptr);

	createTextureImageViews();
}

void icpPrimitiveRendererComponment::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipmapLevel)
{
	auto vulkanRHI = dynamic_pointer_cast<icpVulkanRHI>(g_system_container.m_renderSystem->m_rhi);

	VkCommandBuffer commandBuffer = icpVulkanUtility::beginSingleTimeCommands(vulkanRHI->m_transferCommandPool, vulkanRHI->m_device);

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

	icpVulkanUtility::endSingleTimeCommandsAndSubmit(commandBuffer, vulkanRHI->m_transferQueue, vulkanRHI->m_transferCommandPool, vulkanRHI->m_device);
}

void icpPrimitiveRendererComponment::copyBuffer2Image(VkBuffer srcBuffer, VkImage dstImage, uint32_t width, uint32_t height)
{
	auto vulkanRHI = dynamic_pointer_cast<icpVulkanRHI>(g_system_container.m_renderSystem->m_rhi);

	VkCommandBuffer commandBuffer = icpVulkanUtility::beginSingleTimeCommands(vulkanRHI->m_transferCommandPool, vulkanRHI->m_device);

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
		vulkanRHI->m_transferQueue,
		vulkanRHI->m_transferCommandPool,
		vulkanRHI->m_device
	);
}

void icpPrimitiveRendererComponment::createTextureImageViews()
{
	auto vulkanRHI = dynamic_pointer_cast<icpVulkanRHI>(g_system_container.m_renderSystem->m_rhi);
	m_textureImageView = icpVulkanUtility::createImageView(m_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, 1, vulkanRHI->m_device);
}

void icpPrimitiveRendererComponment::createTextureSampler()
{
	auto vulkanRHI = dynamic_pointer_cast<icpVulkanRHI>(g_system_container.m_renderSystem->m_rhi);

	VkSamplerCreateInfo sampler{};
	sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler.magFilter = VkFilter::VK_FILTER_LINEAR;
	sampler.minFilter = VkFilter::VK_FILTER_LINEAR;

	sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

	sampler.anisotropyEnable = VK_TRUE;

	VkPhysicalDeviceProperties properties;
	vkGetPhysicalDeviceProperties(vulkanRHI->m_physicalDevice, &properties);

	sampler.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
	sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;

	sampler.unnormalizedCoordinates = VK_FALSE;
	sampler.compareEnable = VK_FALSE;
	sampler.compareOp = VK_COMPARE_OP_ALWAYS;

	sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler.mipLodBias = 0.0f;
	sampler.minLod = 0.0f;
	sampler.maxLod = static_cast<float>(1);

	if (vkCreateSampler(vulkanRHI->m_device, &sampler, nullptr, &m_textureSampler) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create sampler!");
	}
}

INCEPTION_END_NAMESPACE