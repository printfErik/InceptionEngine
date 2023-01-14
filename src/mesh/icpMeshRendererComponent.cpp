#include "icpMeshRendererComponent.h"
#include "../core/icpSystemContainer.h"
#include "../render/icpRenderSystem.h"
#include "../render/icpVulkanUtility.h"
#include "../render/icpImageResource.h"
#include "../core/icpLogSystem.h"
#include "../resource/icpResourceSystem.h"
#include "icpMeshResource.h"

INCEPTION_BEGIN_NAMESPACE

void icpMeshRendererComponent::prepareRenderResourceForMesh()
{
	createTextureImages();
	createTextureSampler();

	createVertexBuffers();
	createIndexBuffers();
	createUniformBuffers();

	allocateDescriptorSets();
}

void icpMeshRendererComponent::allocateDescriptorSets()
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

void icpMeshRendererComponent::createTextureImages()
{
	// todo: remove all dynamic_cast
	auto vulkanRHI = dynamic_pointer_cast<icpVulkanRHI>(g_system_container.m_renderSystem->m_rhi);

	if (!m_imgRes)
	{
		m_imgRes = std::dynamic_pointer_cast<icpImageResource>(g_system_container.m_resourceSystem->m_resources.m_allResources[icpResourceType::TEXTURE][m_texResId]);
	}

	if (!m_imgRes)
	{
		ICP_LOG_FATAL("image resource should be valid!");
	}

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMem;

	icpVulkanUtility::createVulkanBuffer(
		m_imgRes->getImgBuffer().size(),
		VK_SHARING_MODE_EXCLUSIVE,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer,
		stagingBufferMem,
		vulkanRHI->m_device,
		vulkanRHI->m_physicalDevice
	);

	void* data;
	vkMapMemory(vulkanRHI->m_device, stagingBufferMem, 0, static_cast<uint32_t>(m_imgRes->getImgBuffer().size()), 0, &data);
	memcpy(data, m_imgRes->getImgBuffer().data(), m_imgRes->getImgBuffer().size());
	vkUnmapMemory(vulkanRHI->m_device, stagingBufferMem);

	icpVulkanUtility::createVulkanImage(
		static_cast<uint32_t>(m_imgRes->m_imgWidth),
		static_cast<uint32_t>(m_imgRes->m_height),
		static_cast<uint32_t>(m_imgRes->m_mipmapLevel),
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_textureImage,
		m_textureBufferMem,
		vulkanRHI->m_device,
		vulkanRHI->m_physicalDevice
	);

	icpVulkanUtility::transitionImageLayout(m_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<uint32_t>(m_imgRes->m_mipmapLevel), vulkanRHI->m_transferCommandPool, vulkanRHI->m_device, vulkanRHI->m_transferQueue);
	icpVulkanUtility::copyBuffer2Image(stagingBuffer, m_textureImage, static_cast<uint32_t>(m_imgRes->m_imgWidth), static_cast<uint32_t>(m_imgRes->m_height), vulkanRHI->m_transferCommandPool, vulkanRHI->m_device, vulkanRHI->m_transferQueue);
	//transitionImageLayout(m_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, static_cast<uint32_t>(imgP->m_mipmapLevel));

	vkDestroyBuffer(vulkanRHI->m_device, stagingBuffer, nullptr);
	vkFreeMemory(vulkanRHI->m_device, stagingBufferMem, nullptr);

	icpVulkanUtility::generateMipmaps(m_textureImage, VK_FORMAT_R8G8B8A8_SRGB, static_cast<uint32_t>(m_imgRes->m_imgWidth), static_cast<uint32_t>(m_imgRes->m_height), static_cast<uint32_t>(m_imgRes->m_mipmapLevel), vulkanRHI->m_graphicsCommandPool, vulkanRHI->m_device, vulkanRHI->m_graphicsQueue, vulkanRHI->m_physicalDevice);

	createTextureImageViews(m_imgRes->m_mipmapLevel);
}

void icpMeshRendererComponent::createTextureSampler()
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

	const auto imgP = m_imgRes;
	sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler.mipLodBias = 0.0f;
	sampler.minLod = 0.0f;
	sampler.maxLod = static_cast<float>(imgP->m_mipmapLevel);

	if (vkCreateSampler(vulkanRHI->m_device, &sampler, nullptr, &m_textureSampler) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create sampler!");
	}
}

void icpMeshRendererComponent::createTextureImageViews(size_t mipmaplevel)
{
	auto vulkanRHI = dynamic_pointer_cast<icpVulkanRHI>(g_system_container.m_renderSystem->m_rhi);
	m_textureImageView = icpVulkanUtility::createImageView(m_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, mipmaplevel, vulkanRHI->m_device);
}

void icpMeshRendererComponent::createUniformBuffers()
{
	auto vulkanRHI = dynamic_pointer_cast<icpVulkanRHI>(g_system_container.m_renderSystem->m_rhi);
	auto bufferSize = sizeof(UBOPerMaterial);

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

void icpMeshRendererComponent::createVertexBuffers()
{
	auto vulkanRHI = dynamic_pointer_cast<icpVulkanRHI>(g_system_container.m_renderSystem->m_rhi);

	const auto meshRes = std::dynamic_pointer_cast<icpMeshResource>(g_system_container.m_resourceSystem->m_resources.m_allResources[icpResourceType::MESH][m_meshResId]);

	auto bufferSize = sizeof(meshRes->m_meshData.m_vertices[0]) * meshRes->m_meshData.m_vertices.size();

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
	memcpy(data, meshRes->m_meshData.m_vertices.data(), (size_t)bufferSize);
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

void icpMeshRendererComponent::createIndexBuffers()
{
	auto vulkanRHI = dynamic_pointer_cast<icpVulkanRHI>(g_system_container.m_renderSystem->m_rhi);

	const auto meshRes = std::dynamic_pointer_cast<icpMeshResource>(g_system_container.m_resourceSystem->m_resources.m_allResources[icpResourceType::MESH][m_meshResId]);
	VkDeviceSize bufferSize = sizeof(meshRes->m_meshData.m_vertexIndices[0]) * meshRes->m_meshData.m_vertexIndices.size();

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
	memcpy(data, meshRes->m_meshData.m_vertexIndices.data(), (size_t)bufferSize);
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


INCEPTION_END_NAMESPACE