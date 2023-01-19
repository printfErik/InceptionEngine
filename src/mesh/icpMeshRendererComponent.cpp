#include "icpMeshRendererComponent.h"
#include "../core/icpSystemContainer.h"
#include "../render/icpRenderSystem.h"
#include "../render/icpVulkanUtility.h"
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
	m_perMeshUniformBufferMems.resize(MAX_FRAMES_IN_FLIGHT);

	VkSharingMode mode = vulkanRHI->m_queueIndices.m_graphicsFamily.value() == vulkanRHI->m_queueIndices.m_transferFamily.value() ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		icpVulkanUtility::createVulkanBuffer(
			perMeshSize,
			mode,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD,
			m_perMeshUniformBuffers[i],
			m_perMeshUniformBufferMems[i],
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

std::shared_ptr<icpMaterialTemplate> icpMeshRendererComponent::addMaterial(eMaterialModel materialType)
{
	auto materialSystem = g_system_container.m_renderSystem->m_materialSystem;
	auto instance = materialSystem->createMaterialInstance(materialType);
	m_materials.push_back(instance);

	return instance;
}


INCEPTION_END_NAMESPACE