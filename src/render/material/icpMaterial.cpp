#include "icpMaterial.h"

#include "icpTextureRenderResourceManager.h"
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

void icpBlinnPhongMaterialInstance::createUniformBuffers()
{
	auto vulkanRHI = dynamic_pointer_cast<icpVulkanRHI>(g_system_container.m_renderSystem->m_rhi);

	auto UBOSize = sizeof(UBOPerMaterial);

	m_perMaterialUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	m_perMaterialUniformBufferMems.resize(MAX_FRAMES_IN_FLIGHT);

	VkSharingMode mode = vulkanRHI->m_queueIndices.m_graphicsFamily.value() == vulkanRHI->m_queueIndices.m_transferFamily.value() ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		icpVulkanUtility::createVulkanBuffer(
			UBOSize,
			mode,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD,
			m_perMaterialUniformBuffers[i],
			m_perMaterialUniformBufferMems[i],
			vulkanRHI->m_device,
			vulkanRHI->m_physicalDevice);
	}
}

void icpBlinnPhongMaterialInstance::allocateDescriptorSets()
{
	auto vulkanRHI = dynamic_pointer_cast<icpVulkanRHI>(g_system_container.m_renderSystem->m_rhi);

	auto& layout = g_system_container.m_renderSystem->m_renderPassManager->accessRenderPass(eRenderPass::MAIN_FORWARD_PASS)->m_DSLayouts[icpMainForwardPass::eMainForwardPassDSType::PER_MATERIAL];
	std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, layout);

	VkDescriptorSetAllocateInfo allocateInfo{};
	allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocateInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
	allocateInfo.descriptorPool = vulkanRHI->m_descriptorPool;
	allocateInfo.pSetLayouts = layouts.data();

	m_perMaterialDSs.resize(MAX_FRAMES_IN_FLIGHT);

	if (vkAllocateDescriptorSets(vulkanRHI->m_device, &allocateInfo, m_perMaterialDSs.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate descriptor sets!");
	}

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = m_perMaterialUniformBuffers[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UBOMeshRenderResource);

		auto texRenderResMgr = g_system_container.m_renderSystem->m_textureRenderResourceManager;
		auto& info = texRenderResMgr->m_textureRenderResurces[m_texRenderResourceIDs[0]];

		VkDescriptorImageInfo imageInfo1{};
		imageInfo1.imageView = info.m_texImageView;
		imageInfo1.sampler = info.m_texSampler;
		imageInfo1.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		info = texRenderResMgr->m_textureRenderResurces[m_texRenderResourceIDs[1]];
		VkDescriptorImageInfo imageInfo2{};
		imageInfo2.imageView = info.m_texImageView;
		imageInfo2.sampler = info.m_texSampler;
		imageInfo2.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		std::array<VkWriteDescriptorSet, 3> descriptorWrites{};
		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = m_perMaterialDSs[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = m_perMaterialDSs[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pImageInfo = &imageInfo1;

		descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[2].dstSet = m_perMaterialDSs[i];
		descriptorWrites[2].dstBinding = 1;
		descriptorWrites[2].dstArrayElement = 0;
		descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[2].descriptorCount = 1;
		descriptorWrites[2].pImageInfo = &imageInfo2;

		vkUpdateDescriptorSets(vulkanRHI->m_device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
	}
}

icpBlinnPhongMaterialInstance::icpBlinnPhongMaterialInstance()
{
	m_materialTemplateType = eMaterialModel::BLINNPHONG;
}


void icpMaterialSubSystem::initializeMaterialSubSystem()
{
	m_materials.resize(static_cast<int>(eMaterialModel::MATERIAL_TYPE_COUNT) - 1);
}


std::shared_ptr<icpMaterialTemplate> icpMaterialSubSystem::createMaterialInstance(eMaterialModel materialType)
{
	switch (materialType)
	{
		case eMaterialModel::BLINNPHONG:
		{
			std::shared_ptr<icpMaterialTemplate> instance = std::make_shared<icpBlinnPhongMaterialInstance>();
			m_materials.push_back(instance);
		}
		break;
		default:
			break;
	}
}

void icpBlinnPhongMaterialInstance::addDiffuseTexture(const std::string& texID)
{
	auto texRendeResMgr = g_system_container.m_renderSystem->m_textureRenderResourceManager;
	if (texRendeResMgr->m_textureRenderResurces.find(texID) == texRendeResMgr->m_textureRenderResurces.end() 
		|| texRendeResMgr->m_textureRenderResurces[texID].m_state == eTextureRenderResouceState::UNINITIALIZED)
	{
		texRendeResMgr->setupTextureRenderResources(texID);
	}
	m_texRenderResourceIDs.push_back(texID);
}

void icpBlinnPhongMaterialInstance::addSpecularTexture(const std::string& texID)
{
	auto texRendeResMgr = g_system_container.m_renderSystem->m_textureRenderResourceManager;
	if (texRendeResMgr->m_textureRenderResurces.find(texID) == texRendeResMgr->m_textureRenderResurces.end()
		|| texRendeResMgr->m_textureRenderResurces[texID].m_state == eTextureRenderResouceState::UNINITIALIZED)
	{
		texRendeResMgr->setupTextureRenderResources(texID);
	}
	m_texRenderResourceIDs.push_back(texID);
}

void icpBlinnPhongMaterialInstance::addShininess(float shininess)
{
	m_ubo.shininess = shininess;
}

void icpBlinnPhongMaterialInstance::setupMaterialRenderResources()
{
	createUniformBuffers();
	allocateDescriptorSets();
}


INCEPTION_END_NAMESPACE