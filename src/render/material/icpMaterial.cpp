#include "icpMaterial.h"

#include "icpTextureRenderResourceManager.h"
#include "../RHI/Vulkan/icpVulkanRHI.h"
#include "../RHI/Vulkan/icpVulkanUtility.h"
#include "../icpRenderSystem.h"
#include "../../core/icpSystemContainer.h"
#include "../../resource/icpResourceSystem.h"
#include "../../core/icpLogSystem.h"
#include "../../mesh/icpMeshResource.h"
#include "../icpImageResource.h"
#include "../renderPass/icpMainForwardPass.h"

INCEPTION_BEGIN_NAMESPACE

icpMaterialInstance::icpMaterialInstance(eMaterialShadingModel shading_model)
{
	m_shadingModel = shading_model;
}

void icpMaterialInstance::CreateUniformBuffers()
{
	auto vulkanRHI = dynamic_pointer_cast<icpVulkanRHI>(g_system_container.m_renderSystem->m_rhi);

	uint32_t UBOSize = ComputeUBOSize();

	if (UBOSize > 0)
	{
		m_perMaterialUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		m_perMaterialUniformBufferAllocations.resize(MAX_FRAMES_IN_FLIGHT);

		VkSharingMode mode = vulkanRHI->m_queueIndices.m_graphicsFamily.value() == vulkanRHI->m_queueIndices.m_transferFamily.value() ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			icpVulkanUtility::CreateGPUBuffer(
				UBOSize,
				mode,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				vulkanRHI->m_vmaAllocator,
				m_perMaterialUniformBufferAllocations[i],
				m_perMaterialUniformBuffers[i]
			);
		}
	}
}

void icpMaterialInstance::AllocateDescriptorSets()
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
		uint64_t UBOSize = ComputeUBOSize();
		VkDescriptorBufferInfo bufferInfo{};
		if (UBOSize > 0)
		{
			bufferInfo.buffer = m_perMaterialUniformBuffers[i];
			bufferInfo.offset = 0;
			bufferInfo.range = UBOSize;
		}
		
		auto texRenderResMgr = g_system_container.m_renderSystem->m_textureRenderResourceManager;

		std::vector<VkDescriptorImageInfo> imageInfos;
		for (auto& texture : m_textureParameterValues)
		{
			auto& info = texRenderResMgr->m_textureRenderResurces[texture];

			VkDescriptorImageInfo imageInfo{};
			imageInfo.imageView = info.m_texImageView;
			imageInfo.sampler = info.m_texSampler;
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			imageInfos.push_back(imageInfo);
		}

		uint32_t bindingNumber = (UBOSize > 0 ? 1u : 0u) + m_textureParameterValues.size();

		std::vector<VkWriteDescriptorSet> descriptorWrites(bindingNumber);

		uint32_t index = 0;
		if (UBOSize > 0)
		{
			descriptorWrites[index].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[index].dstSet = m_perMaterialDSs[i];
			descriptorWrites[index].dstBinding = index;
			descriptorWrites[index].dstArrayElement = 0;
			descriptorWrites[index].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[index].descriptorCount = 1;
			descriptorWrites[index].pBufferInfo = &bufferInfo;
			index++;
		}
		
		for (auto& info : imageInfos)
		{
			descriptorWrites[index].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[index].dstSet = m_perMaterialDSs[i];
			descriptorWrites[index].dstBinding = index;
			descriptorWrites[index].dstArrayElement = 0;
			descriptorWrites[index].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[index].descriptorCount = 1;
			descriptorWrites[index].pImageInfo = &info;
		}

		vkUpdateDescriptorSets(vulkanRHI->m_device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
	}
}

void icpMaterialSubSystem::initializeMaterialSubSystem()
{
	//m_materials.resize(static_cast<int>(eMaterialModel::MATERIAL_TYPE_COUNT) - 1);
}


std::shared_ptr<icpMaterialTemplate> icpMaterialSubSystem::createMaterialInstance(eMaterialShadingModel shadingModel)
{
	std::shared_ptr<icpMaterialTemplate> instance = std::make_shared<icpMaterialInstance>(shadingModel);
	m_vMaterialContainer.push_back(instance);
	return instance;
}

/*
void icpMaterialInstance::addDiffuseTexture(const std::string& texID)
{
	auto texRendeResMgr = g_system_container.m_renderSystem->m_textureRenderResourceManager;
	if (texRendeResMgr->m_textureRenderResurces.find(texID) == texRendeResMgr->m_textureRenderResurces.end() 
		|| texRendeResMgr->m_textureRenderResurces[texID].m_state == eTextureRenderResouceState::UNINITIALIZED)
	{
		texRendeResMgr->setupTextureRenderResources(texID);
	}
	m_textureParameterValues.push_back(texID);
}

void icpMaterialInstance::addSpecularTexture(const std::string& texID)
{
	auto texRendeResMgr = g_system_container.m_renderSystem->m_textureRenderResourceManager;
	if (texRendeResMgr->m_textureRenderResurces.find(texID) == texRendeResMgr->m_textureRenderResurces.end()
		|| texRendeResMgr->m_textureRenderResurces[texID].m_state == eTextureRenderResouceState::UNINITIALIZED)
	{
		texRendeResMgr->setupTextureRenderResources(texID);
	}
	m_textureParameterValues.push_back(texID);
}
*/

void icpMaterialInstance::AddTexture(const std::string& texID)
{
	auto texRendeResMgr = g_system_container.m_renderSystem->m_textureRenderResourceManager;

	if(texRendeResMgr->m_textureRenderResurces.find(texID) == texRendeResMgr->m_textureRenderResurces.end()
		|| texRendeResMgr->m_textureRenderResurces[texID].m_state == eTextureRenderResouceState::UNINITIALIZED)
	{
		texRendeResMgr->setupTextureRenderResources(texID);
	}

	icpTextureParameterInfo info{};
	info.m_textureID = texID;
	info.m_strTextureName = texID;

	m_textureParameterValues.push_back(info);
}


void icpMaterialInstance::addShininess(float shininess)
{
	//m_ubo.shininess = shininess;
}

uint64_t icpMaterialInstance::ComputeUBOSize()
{
	uint64_t totalSize = 0;
	for (auto bVal : m_boolParameterValues)
	{
		totalSize += sizeof(float);
	}

	for (auto fVal : m_scalarParameterValues)
	{
		totalSize += sizeof(float);
	}

	for (auto vVal : m_vectorParameterValues)
	{
		totalSize += sizeof(glm::vec4);
	}

	return totalSize;
}


void icpMaterialInstance::setupMaterialRenderResources()
{
	CreateUniformBuffers();
	AllocateDescriptorSets();
}

void* icpMaterialInstance::MapUniformBuffer(int index)
{
	void* materialData;
	auto vulkanRHI = dynamic_pointer_cast<icpVulkanRHI>(g_system_container.m_renderSystem->m_rhi);
	vmaMapMemory(vulkanRHI->m_vmaAllocator, m_perMaterialUniformBufferAllocations[index], &materialData);
	return materialData;
}


INCEPTION_END_NAMESPACE