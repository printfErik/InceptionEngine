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

	icpDescriptorSetCreation creation;
	auto& layout = g_system_container.m_renderSystem->GetSceneRenderer()
		->AccessRenderPass(eRenderPass::MAIN_FORWARD_PASS)
		->m_DSLayouts[icpMainForwardPass::eMainForwardPassDSType::PER_MESH];

	creation.layoutInfo = layout;

	std::vector<icpBufferRenderResourceInfo> bufferInfos;
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		icpBufferRenderResourceInfo bufferInfo{};
		bufferInfo.buffer = m_perMeshUniformBuffers[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UBOMeshRenderResource);
		bufferInfos.push_back(bufferInfo);
	}

	creation.SetUniformBuffer(0, bufferInfos);
	pGPUDevice->CreateDescriptorSet(creation, m_perMeshDSs);
}

void icpMeshRendererComponent::createUniformBuffers()
{
	auto vulkanRHI = g_system_container.m_renderSystem->GetGPUDevice();

	auto perMeshSize = sizeof(UBOMeshRenderResource);

	m_perMeshUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	m_perMeshUniformBufferAllocations.resize(MAX_FRAMES_IN_FLIGHT);

	VkSharingMode mode = vulkanRHI->GetQueueFamilyIndices().m_graphicsFamily.value() == vulkanRHI->GetQueueFamilyIndices().m_transferFamily.value() ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;
	auto& queueIndices = vulkanRHI->GetQueueFamilyIndicesVector();

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		icpVulkanUtility::CreateGPUBuffer(
			perMeshSize,
			mode,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			vulkanRHI->GetVmaAllocator(),
			m_perMeshUniformBufferAllocations[i],
			m_perMeshUniformBuffers[i],
			queueIndices.size(),
			queueIndices.data()
		);
	}
}

void icpMeshRendererComponent::createVertexBuffers()
{
	auto vulkanRHI = g_system_container.m_renderSystem->GetGPUDevice();

	const auto meshRes = std::dynamic_pointer_cast<icpMeshResource>(g_system_container.m_resourceSystem->GetResourceContainer()[icpResourceType::MESH][m_meshResId]);

	auto bufferSize = sizeof(meshRes->m_meshData.m_vertices[0]) * meshRes->m_meshData.m_vertices.size();

	VkBuffer stagingBuffer{ VK_NULL_HANDLE };
	VmaAllocation stagingBufferAllocation{ VK_NULL_HANDLE };

	VkSharingMode mode = vulkanRHI->GetQueueFamilyIndices().m_graphicsFamily.value() == vulkanRHI->GetQueueFamilyIndices().m_transferFamily.value() ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;
	auto& queueIndices = vulkanRHI->GetQueueFamilyIndicesVector();
	icpVulkanUtility::CreateGPUBuffer(
		bufferSize,
		mode,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		vulkanRHI->GetVmaAllocator(),
		stagingBufferAllocation,
		stagingBuffer,
		queueIndices.size(),
		queueIndices.data()
	);

	void* data;
	vmaMapMemory(vulkanRHI->GetVmaAllocator(), stagingBufferAllocation, &data);
	memcpy(data, meshRes->m_meshData.m_vertices.data(), (size_t)bufferSize);
	vmaUnmapMemory(vulkanRHI->GetVmaAllocator(), stagingBufferAllocation);

	icpVulkanUtility::CreateGPUBuffer(
		bufferSize,
		mode,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		vulkanRHI->GetVmaAllocator(),
		m_vertexBufferAllocation,
		m_vertexBuffer,
		queueIndices.size(),
		queueIndices.data()
	);

	icpVulkanUtility::copyBuffer(stagingBuffer,
		m_vertexBuffer,
		bufferSize,
		vulkanRHI->GetLogicalDevice(),
		vulkanRHI->GetTransferCommandPool(),
		vulkanRHI->GetTransferQueue()
	);

	vmaDestroyBuffer(vulkanRHI->GetVmaAllocator(), stagingBuffer, stagingBufferAllocation);
}

void icpMeshRendererComponent::createIndexBuffers()
{
	auto vulkanRHI = g_system_container.m_renderSystem->GetGPUDevice();

	const auto meshRes = std::dynamic_pointer_cast<icpMeshResource>(g_system_container.m_resourceSystem->GetResourceContainer()[icpResourceType::MESH][m_meshResId]);
	VkDeviceSize bufferSize = sizeof(meshRes->m_meshData.m_vertexIndices[0]) * meshRes->m_meshData.m_vertexIndices.size();

	VkBuffer stagingBuffer{ VK_NULL_HANDLE };
	VmaAllocation stagingBufferAllocation{ VK_NULL_HANDLE };

	VkSharingMode mode = vulkanRHI->GetQueueFamilyIndices().m_graphicsFamily.value() == vulkanRHI->GetQueueFamilyIndices().m_transferFamily.value() ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;
	auto& queueIndices = vulkanRHI->GetQueueFamilyIndicesVector();
	icpVulkanUtility::CreateGPUBuffer(
		bufferSize,
		mode,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		vulkanRHI->GetVmaAllocator(),
		stagingBufferAllocation,
		stagingBuffer,
		queueIndices.size(),
		queueIndices.data()
	);

	void* data;
	vmaMapMemory(vulkanRHI->GetVmaAllocator(), stagingBufferAllocation, &data);
	memcpy(data, meshRes->m_meshData.m_vertexIndices.data(), (size_t)bufferSize);
	vmaUnmapMemory(vulkanRHI->GetVmaAllocator(), stagingBufferAllocation);

	icpVulkanUtility::CreateGPUBuffer(
		bufferSize,
		mode,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		vulkanRHI->GetVmaAllocator(),
		m_indexBufferAllocation,
		m_indexBuffer,
		queueIndices.size(),
		queueIndices.data()
	);

	icpVulkanUtility::copyBuffer(stagingBuffer,
		m_indexBuffer,
		bufferSize,
		vulkanRHI->GetLogicalDevice(),
		vulkanRHI->GetTransferCommandPool(),
		vulkanRHI->GetTransferQueue()
	);

	vmaDestroyBuffer(vulkanRHI->GetVmaAllocator(), stagingBuffer,stagingBufferAllocation);
}

std::shared_ptr<icpMaterialTemplate> icpMeshRendererComponent::addMaterial(eMaterialShadingModel shadingModel)
{
	if (m_pMaterial)
	{
		m_pMaterial.reset();
	}
	auto materialSystem = g_system_container.m_renderSystem->GetMaterialSubSystem();
	auto instance = materialSystem->createMaterialInstance(shadingModel);
	m_pMaterial = instance;

	return instance;
}

void icpMeshRendererComponent::AddMaterial(std::shared_ptr<icpMaterialTemplate> material)
{
	if (m_pMaterial)
	{
		m_pMaterial.reset();
	}
	m_pMaterial = material;
}

INCEPTION_END_NAMESPACE