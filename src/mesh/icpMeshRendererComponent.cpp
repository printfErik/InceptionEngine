#include "icpMeshRendererComponent.h"
#include "../core/icpSystemContainer.h"
#include "../render/icpRenderSystem.h"
#include "../render/RHI/Vulkan/icpVulkanUtility.h"
#include "../render/icpImageResource.h"
#include "../core/icpLogSystem.h"
#include "../resource/icpResourceSystem.h"
#include "icpMeshResource.h"
#include "../render/renderPass/icpEditorUiPass.h"
#include "../render/renderPass/icpMainForwardPass.h"
#include "../render/RHI/icpDescirptorSet.h"

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
	auto pGPUDevice = g_system_container.m_renderSystem->GetGPUDevice();

	auto& layout = g_system_container.m_renderSystem->GetRenderPassManager()->accessRenderPass(eRenderPass::MAIN_FORWARD_PASS)->m_DSLayouts[icpMainForwardPass::eMainForwardPassDSType::PER_MESH];
	std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, layout);

	icpDescriptorSetCreation creation;

	creation.layout = layout;
	pGPUDevice->CreateDescriptorSet(creation, m_perMeshDSs);
}

void icpMeshRendererComponent::createUniformBuffers()
{
	auto vulkanRHI = dynamic_pointer_cast<icpVkGPUDevice>(g_system_container.m_renderSystem->m_rhi);

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
	auto vulkanRHI = dynamic_pointer_cast<icpVkGPUDevice>(g_system_container.m_renderSystem->m_rhi);

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
	auto vulkanRHI = dynamic_pointer_cast<icpVkGPUDevice>(g_system_container.m_renderSystem->m_rhi);

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