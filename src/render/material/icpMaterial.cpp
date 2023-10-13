#include "icpMaterial.h"

#include "icpTextureRenderResourceManager.h"
#include "../RHI/Vulkan/icpVkGPUDevice.h"
#include "../RHI/Vulkan/icpVulkanUtility.h"
#include "../icpRenderSystem.h"
#include "../../core/icpSystemContainer.h"
#include "../../resource/icpResourceSystem.h"
#include "../../core/icpLogSystem.h"
#include "../../mesh/icpMeshResource.h"
#include "../icpImageResource.h"
#include "../renderPass/icpMainForwardPass.h"
#include "../../core/icpConfigSystem.h"

INCEPTION_BEGIN_NAMESPACE

icpMaterialInstance::icpMaterialInstance(eMaterialShadingModel shading_model)
{
	m_shadingModel = shading_model;
}

void icpMaterialInstance::CreateUniformBuffers()
{
	auto vulkanRHI = g_system_container.m_renderSystem->GetGPUDevice();

	uint32_t UBOSize = ComputeUBOSize();

	if (UBOSize > 0)
	{
		m_perMaterialUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		m_perMaterialUniformBufferAllocations.resize(MAX_FRAMES_IN_FLIGHT);

		VkSharingMode mode = vulkanRHI->GetQueueFamilyIndices().m_graphicsFamily.value() == vulkanRHI->GetQueueFamilyIndices().m_transferFamily.value() ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			icpVulkanUtility::CreateGPUBuffer(
				UBOSize,
				mode,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				vulkanRHI->GetVmaAllocator(),
				m_perMaterialUniformBufferAllocations[i],
				m_perMaterialUniformBuffers[i]
			);
		}
	}
}

void icpMaterialInstance::AllocateDescriptorSets()
{
	auto vulkanRHI = g_system_container.m_renderSystem->GetGPUDevice();

	icpDescriptorSetCreation creation{};
	auto& layout = g_system_container.m_renderSystem->GetRenderPassManager()
		->accessRenderPass(eRenderPass::MAIN_FORWARD_PASS)
		->m_DSLayouts[icpMainForwardPass::eMainForwardPassDSType::PER_MATERIAL];

	creation.layoutInfo = layout;

	std::vector<icpBufferRenderResourceInfo> bufferInfos;

	uint64_t UBOSize = ComputeUBOSize();
	if (UBOSize > 0)
	{
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			icpBufferRenderResourceInfo bufferInfo{};
			bufferInfo.buffer = m_perMaterialUniformBuffers[i];
			bufferInfo.offset = 0;
			bufferInfo.range = UBOSize;

			bufferInfos.push_back(bufferInfo);
		}
	}

	creation.SetUniformBuffer(0, bufferInfos);

	std::vector<std::vector<icpTextureRenderResourceInfo>> imgInfosAllFrames;
	auto texRenderResMgr = g_system_container.m_renderSystem->GetTextureRenderResourceManager();

	std::vector<icpTextureRenderResourceInfo> imageInfos;
	for (auto& texture : m_vTextureParameterValues)
	{
		auto& info = texRenderResMgr->m_textureRenderResources[texture.m_textureID];

		icpTextureRenderResourceInfo imageInfo{};
		imageInfo.m_texImageView = info.m_texImageView;
		imageInfo.m_texSampler = info.m_texSampler;
		imageInfo.m_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			imageInfos.push_back(imageInfo);
		}

		imgInfosAllFrames.push_back(imageInfos);
	}

	for (uint32_t i = 0; i < imgInfosAllFrames.size(); i++)
	{
		creation.SetCombinedImageSampler(i + 1, imgInfosAllFrames[i]);
	}

	vulkanRHI->CreateDescriptorSet(creation, m_perMaterialDSs);
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

void icpMaterialInstance::AddScalaValue(const icpScalaMaterialParameterInfo& value)
{
	m_vScalarParameterValues.push_back(value);
}


void icpMaterialInstance::AddTexture(const std::string& texID)
{
	auto texRendeResMgr = g_system_container.m_renderSystem->GetTextureRenderResourceManager();

	if(texRendeResMgr->m_textureRenderResources.find(texID) == texRendeResMgr->m_textureRenderResources.end())
	{
		auto imgPath = g_system_container.m_configSystem->m_imageResourcePath / (texID + ".png");
		g_system_container.m_resourceSystem->loadImageResource(imgPath);
	}

	if (texRendeResMgr->m_textureRenderResources[texID].m_state == eTextureRenderResouceState::UNINITIALIZED)
	{
		texRendeResMgr->setupTextureRenderResources(texID);
	}

	icpTextureMaterialParameterInfo info{};
	info.m_textureID = texID;
	info.m_strTextureName = texID;

	m_vTextureParameterValues.push_back(info);
}


uint64_t icpMaterialInstance::ComputeUBOSize()
{
	uint64_t totalSize = 0;
	for (auto bVal : m_vBoolParameterValues)
	{
		totalSize += sizeof(float);
	}

	for (auto fVal : m_vScalarParameterValues)
	{
		totalSize += sizeof(float);
	}

	for (auto vVal : m_vVectorParameterValues)
	{
		totalSize += sizeof(glm::vec4);
	}

	return totalSize;
}

uint32_t icpMaterialInstance::GetSRVNumber() const
{
	return m_nSRVs;
}

void icpMaterialInstance::SetupMaterialRenderResources()
{
	CreateUniformBuffers();
	AllocateDescriptorSets();
}


INCEPTION_END_NAMESPACE