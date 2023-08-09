#include "icpMeshRendererComponent.h"
#include "../core/icpSystemContainer.h"
#include "../render/icpRenderSystem.h"
#include "../render/RHI/Vulkan/icpVulkanUtility.h"
#include "../render/icpImageResource.h"
#include "../core/icpLogSystem.h"
#include "../resource/icpResourceSystem.h"
#include "icpMeshResource.h"
#include "../render/renderPass/icpMainForwardPass.h"

INCEPTION_BEGIN_NAMESPACE

void icpMeshRendererComponent::prepareRenderResourceForMesh()
{
	createVertexBuffers();
	createIndexBuffers();
	createUniformBuffers();

	allocateDescriptorSets();
}

void icpMeshRendererComponent::allocateDescriptorSets()
{
	auto vulkanRHI = dynamic_pointer_cast<icpVulkanRHI>(g_system_container.m_renderSystem->m_rhi);

	auto& layout = g_system_container.m_renderSystem->m_renderPassManager->accessRenderPass(eRenderPass::MAIN_FORWARD_PASS)->m_DSLayouts[icpMainForwardPass::eMainForwardPassDSType::PER_MESH];
	std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, layout);

	VkDescriptorSetAllocateInfo allocateInfo{};
	allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocateInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
	allocateInfo.descriptorPool = vulkanRHI->m_descriptorPool;
	allocateInfo.pSetLayouts = layouts.data();

	m_perMeshDSs.resize(MAX_FRAMES_IN_FLIGHT);

	if (vkAllocateDescriptorSets(vulkanRHI->m_device, &allocateInfo, m_perMeshDSs.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate descriptor sets!");
	}

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = m_perMeshUniformBuffers[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UBOMeshRenderResource);

		std::array<VkWriteDescriptorSet, 1> descriptorWrites{};
		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = m_perMeshDSs[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;

		vkUpdateDescriptorSets(vulkanRHI->m_device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
	}
}

void icpMeshRendererComponent::createUniformBuffers()
{
	auto vulkanRHI = dynamic_pointer_cast<icpVulkanRHI>(g_system_container.m_renderSystem->m_rhi);

	auto perMeshSize = sizeof(UBOMeshRenderResource);

	m_perMeshUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	m_perMeshUniformBufferAllocations.resize(MAX_FRAMES_IN_FLIGHT);

	VkSharingMode mode = vulkanRHI->m_queueIndices.m_graphicsFamily.value() == vulkanRHI->m_queueIndices.m_transferFamily.value() ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		icpVulkanUtility::CreateGPUBuffer(
			perMeshSize,
			mode,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			vulkanRHI->m_vmaAllocator,
			m_perMeshUniformBufferAllocations[i],
			m_perMeshUniformBuffers[i]
		);
	}
}

void icpMeshRendererComponent::createVertexBuffers()
{
	auto vulkanRHI = dynamic_pointer_cast<icpVulkanRHI>(g_system_container.m_renderSystem->m_rhi);

	const auto meshRes = std::dynamic_pointer_cast<icpMeshResource>(g_system_container.m_resourceSystem->m_resources.m_allResources[icpResourceType::MESH][m_meshResId]);

	auto bufferSize = sizeof(meshRes->m_meshData.m_vertices[0]) * meshRes->m_meshData.m_vertices.size();

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
	memcpy(data, meshRes->m_meshData.m_vertices.data(), (size_t)bufferSize);
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

void icpMeshRendererComponent::createIndexBuffers()
{
	auto vulkanRHI = dynamic_pointer_cast<icpVulkanRHI>(g_system_container.m_renderSystem->m_rhi);

	const auto meshRes = std::dynamic_pointer_cast<icpMeshResource>(g_system_container.m_resourceSystem->m_resources.m_allResources[icpResourceType::MESH][m_meshResId]);
	VkDeviceSize bufferSize = sizeof(meshRes->m_meshData.m_vertexIndices[0]) * meshRes->m_meshData.m_vertexIndices.size();

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
	memcpy(data, meshRes->m_meshData.m_vertexIndices.data(), (size_t)bufferSize);
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

	vmaDestroyBuffer(vulkanRHI->m_vmaAllocator, stagingBuffer,stagingBufferAllocation);
}

std::shared_ptr<icpMaterialTemplate> icpMeshRendererComponent::addMaterial(eMaterialShadingModel shadingModel)
{
	auto& materialSystem = g_system_container.m_renderSystem->m_materialSystem;
	auto instance = materialSystem->createMaterialInstance(shadingModel);
	m_materials.push_back(instance);

	return instance;
}


INCEPTION_END_NAMESPACE