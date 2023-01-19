#include "icpMaterial.h"
#include "../icpVulkanRHI.h"
#include "../icpVulkanUtility.h"
#include "../icpRenderSystem.h"
#include "../../core/icpSystemContainer.h"
#include "../../resource/icpResourceSystem.h"
#include "../../core/icpLogSystem.h"
#include "../../mesh/icpMeshResource.h"
#include "../icpImageResource.h"
#include "../renderPass/icpMainForwardPass.h"

INCEPTION_BEGIN_NAMESPACE

icpLambertMaterialInstance::icpLambertMaterialInstance()
{
	m_materialTemplateType = eMaterialModel::LAMBERT;
}

void icpLambertMaterialInstance::createUniformBuffers()
{
	auto vulkanRHI = dynamic_pointer_cast<icpVulkanRHI>(g_system_container.m_renderSystem->m_rhi);

	auto UBOSize = sizeof(UBOPerMaterial);

	VkSharingMode mode = vulkanRHI->m_queueIndices.m_graphicsFamily.value() == vulkanRHI->m_queueIndices.m_transferFamily.value() ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;

	icpVulkanUtility::createVulkanBuffer(
		UBOSize,
		mode,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD,
		m_perMaterialUniformBuffers,
		m_perMaterialUniformBufferMem,
		vulkanRHI->m_device,
		vulkanRHI->m_physicalDevice);
}


void icpLambertMaterialInstance::allocateDescriptorSets()
{
	auto vulkanRHI = dynamic_pointer_cast<icpVulkanRHI>(g_system_container.m_renderSystem->m_rhi);

	VkDescriptorSetAllocateInfo allocateInfo{};
	allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocateInfo.descriptorSetCount = 1;
	allocateInfo.descriptorPool = vulkanRHI->m_descriptorPool;

	auto& layout = g_system_container.m_renderSystem->m_renderPassManager->accessRenderPass(eRenderPass::MAIN_FORWARD_PASS)->m_DSLayouts[icpMainForwardPass::eMainForwardPassDSType::PER_MATERIAL];

	allocateInfo.pSetLayouts = &layout;

	if (vkAllocateDescriptorSets(vulkanRHI->m_device, &allocateInfo, &m_perMaterialDS) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate descriptor sets!");
	}

	VkDescriptorBufferInfo bufferInfo{};
	bufferInfo.buffer = m_perMaterialUniformBuffers;
	bufferInfo.offset = 0;
	bufferInfo.range = sizeof(UBOMeshRenderResource);

	std::array<VkWriteDescriptorSet, 1> descriptorWrites{};
	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstSet = m_perMeshDS;
	descriptorWrites[0].dstBinding = 0;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pBufferInfo = &bufferInfo;

	vkUpdateDescriptorSets(vulkanRHI->m_device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
}



INCEPTION_END_NAMESPACE