#include "icpPrimitiveRendererComponent.h"
#include "../core/icpSystemContainer.h"
#include "../core/icpLogSystem.h"
#include "../render/icpRenderSystem.h"
#include "../render/RHI/Vulkan/icpVulkanUtility.h"
#include "../mesh/icpMeshData.h"
#include "../render/RHI/Vulkan/vk_mem_alloc.h"
#include "../render/renderPass/icpMainForwardPass.h"

INCEPTION_BEGIN_NAMESPACE

void icpPrimitiveRendererComponent::fillInPrimitiveData(const glm::vec3& color)
{
	switch (m_primitive)
	{
	case ePrimitiveType::CUBE:
	{
		std::vector<icpVertex> cubeVertices{
		{{1,1,1}, color ,{1, 1, 1 }, {-1,-1}},
		{{-1,1,1},color,{-1, 1, 1 }, {-1,-1}},
		{{-1,1,-1},color,{-1,1,-1},{-1,-1}},
		{{1,1,-1},color,{1,1,-1},{-1,-1}},
		{{1,-1,1},color,{1,-1,1},{-1,-1}},
		{{-1,-1,1},color,{-1,-1,1},{-1,-1}},
		{{-1,-1,-1},color,{-1,-1,-1},{-1,-1}},
		{{1,-1,-1},color,{-1,-1,-1},{-1,-1}},
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

void icpPrimitiveRendererComponent::createVertexBuffers()
{
	auto vulkanRHI = dynamic_pointer_cast<icpVulkanRHI>(g_system_container.m_renderSystem->m_rhi);

	auto bufferSize = sizeof(m_vertices[0]) * m_vertices.size();

	VkBuffer stagingBuffer;
	VmaAllocation stagingBufferAllocation;

	VkSharingMode mode = vulkanRHI->m_queueIndices.m_graphicsFamily.value() == vulkanRHI->m_queueIndices.m_transferFamily.value() ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;

	icpVulkanUtility::CreateGPUBuffer(
		bufferSize,
		mode,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		vulkanRHI->m_vmaAllocator,
		stagingBufferAllocation,
		stagingBuffer
	);

	void* data;
	vmaMapMemory(vulkanRHI->m_vmaAllocator, stagingBufferAllocation, &data);
	memcpy(data, m_vertices.data(), (size_t)bufferSize);
	vmaUnmapMemory(vulkanRHI->m_vmaAllocator, stagingBufferAllocation);

	icpVulkanUtility::CreateGPUBuffer(
		bufferSize,
		mode,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		vulkanRHI->m_vmaAllocator,
		m_vertexBufferAllocation,
		m_vertexBuffer
	);

	icpVulkanUtility::copyBuffer(stagingBuffer,
		m_vertexBuffer,
		bufferSize,
		vulkanRHI->m_device,
		vulkanRHI->m_transferCommandPool,
		vulkanRHI->m_transferQueue
	);

	vmaDestroyBuffer(vulkanRHI->m_vmaAllocator, stagingBuffer, stagingBufferAllocation);
	
}

void icpPrimitiveRendererComponent::createIndexBuffers()
{
	auto vulkanRHI = dynamic_pointer_cast<icpVulkanRHI>(g_system_container.m_renderSystem->m_rhi);
	VkDeviceSize bufferSize = sizeof(m_vertexIndices[0]) * m_vertexIndices.size();

	VkBuffer stagingBuffer{ VK_NULL_HANDLE };
	VmaAllocation stagingBufferAllocation{ VK_NULL_HANDLE };

	VkSharingMode mode = vulkanRHI->m_queueIndices.m_graphicsFamily.value() == vulkanRHI->m_queueIndices.m_transferFamily.value() ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;

	icpVulkanUtility::CreateGPUBuffer(
		bufferSize,
		mode,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		vulkanRHI->m_vmaAllocator,
		stagingBufferAllocation,
		stagingBuffer
	);

	void* data;
	vmaMapMemory(vulkanRHI->m_vmaAllocator, stagingBufferAllocation, &data);
	memcpy(data, m_vertexIndices.data(), (size_t)bufferSize);
	vmaUnmapMemory(vulkanRHI->m_vmaAllocator, stagingBufferAllocation);

	icpVulkanUtility::CreateGPUBuffer(
		bufferSize,
		mode,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		vulkanRHI->m_vmaAllocator,
		m_indexBufferAllocation,
		m_indexBuffer
	);

	icpVulkanUtility::copyBuffer(stagingBuffer,
		m_indexBuffer,
		bufferSize,
		vulkanRHI->m_device,
		vulkanRHI->m_transferCommandPool,
		vulkanRHI->m_transferQueue
	);

	vmaDestroyBuffer(vulkanRHI->m_vmaAllocator, stagingBuffer, stagingBufferAllocation);
}

void icpPrimitiveRendererComponent::allocateDescriptorSets()
{
	
	auto vulkanRHI = dynamic_pointer_cast<icpVulkanRHI>(g_system_container.m_renderSystem->m_rhi);

	auto& layout = g_system_container.m_renderSystem->m_renderPassManager->accessRenderPass(eRenderPass::MAIN_FORWARD_PASS)->m_DSLayouts[icpMainForwardPass::eMainForwardPassDSType::PER_MESH];
	std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, layout);

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
		bufferInfo.range = sizeof(UBOMeshRenderResource);

		std::array<VkWriteDescriptorSet, 1> descriptorWrites{};
		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = m_descriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;

		vkUpdateDescriptorSets(vulkanRHI->m_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
	
}

void icpPrimitiveRendererComponent::createUniformBuffers()
{
	auto vulkanRHI = dynamic_pointer_cast<icpVulkanRHI>(g_system_container.m_renderSystem->m_rhi);
	auto bufferSize = sizeof(UBOMeshRenderResource);

	m_uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	m_uniformBufferAllocations.resize(MAX_FRAMES_IN_FLIGHT);

	VkSharingMode mode = vulkanRHI->m_queueIndices.m_graphicsFamily.value() == vulkanRHI->m_queueIndices.m_transferFamily.value() ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		icpVulkanUtility::CreateGPUBuffer(
			bufferSize,
			mode,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			vulkanRHI->m_vmaAllocator,
			m_uniformBufferAllocations[i],
			m_uniformBuffers[i]
		);
	}
}

void icpPrimitiveRendererComponent::createTextureImages()
{
	auto vulkanRHI = dynamic_pointer_cast<icpVulkanRHI>(g_system_container.m_renderSystem->m_rhi);

	VkBuffer stagingBuffer{ VK_NULL_HANDLE };
	VmaAllocation stagingBufferAllocation { VK_NULL_HANDLE };

	std::vector<char> emptyImgData{0, 0, 0, 0};

	icpVulkanUtility::CreateGPUBuffer(
		4, //1 * 1 * 4 rgba
		VK_SHARING_MODE_EXCLUSIVE,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		vulkanRHI->m_vmaAllocator,
		stagingBufferAllocation,
		stagingBuffer
	);

	void* data;
	vmaMapMemory(vulkanRHI->m_vmaAllocator, stagingBufferAllocation, &data);
	memcpy(data, emptyImgData.data(), 4);
	vmaUnmapMemory(vulkanRHI->m_vmaAllocator, stagingBufferAllocation);

	icpVulkanUtility::CreateGPUImage(
		static_cast<uint32_t>(1),
		static_cast<uint32_t>(1),
		static_cast<uint32_t>(1),
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
		vulkanRHI->m_vmaAllocator,
		m_textureImage,
		m_textureBufferAllocation
	);

	icpVulkanUtility::transitionImageLayout(m_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<uint32_t>(1), vulkanRHI->m_transferCommandPool, vulkanRHI->m_device, vulkanRHI->m_transferQueue);
	icpVulkanUtility::copyBuffer2Image(stagingBuffer, m_textureImage, static_cast<uint32_t>(1), static_cast<uint32_t>(1), vulkanRHI->m_transferCommandPool, vulkanRHI->m_device, vulkanRHI->m_transferQueue);
	icpVulkanUtility::transitionImageLayout(m_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1, vulkanRHI->m_transferCommandPool, vulkanRHI->m_device, vulkanRHI->m_transferQueue);

	vmaDestroyBuffer(vulkanRHI->m_vmaAllocator, stagingBuffer, stagingBufferAllocation);

	createTextureImageViews();
}

void icpPrimitiveRendererComponent::createTextureImageViews()
{
	auto vulkanRHI = dynamic_pointer_cast<icpVulkanRHI>(g_system_container.m_renderSystem->m_rhi);
	m_textureImageView = icpVulkanUtility::CreateGPUImageView(m_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, 1, vulkanRHI->m_device);
}

void icpPrimitiveRendererComponent::createTextureSampler()
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

std::shared_ptr<icpMaterialTemplate> icpPrimitiveRendererComponent::AddMaterial(eMaterialShadingModel shading_model)
{
	auto& materialSystem = g_system_container.m_renderSystem->m_materialSystem;
	auto instance = materialSystem->createMaterialInstance(shading_model);
	m_vMaterials.push_back(instance);

	return instance;
}


INCEPTION_END_NAMESPACE