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
		auto& info = texRenderResMgr->m_textureRenderResources[texture.second.m_textureID];

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
		creation.SetCombinedImageSampler(i + 1u, imgInfosAllFrames[i]);
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


void icpMaterialInstance::AddScalaValue(const std::string& key, const icpScalaMaterialParameterInfo& value)
{
	m_vScalarParameterValues[key] = value;
}

void icpMaterialInstance::AddVector4Value(const std::string& key, const icpVector4MaterialParameterInfo& value)
{
	m_vVectorParameterValues[key] = value;
}


void icpMaterialInstance::AddTexture(const std::string& key, const icpTextureMaterialParameterInfo& textureInfo)
{
	auto texRendeResMgr = g_system_container.m_renderSystem->GetTextureRenderResourceManager();

	auto& texID = textureInfo.m_textureID;
	if(texRendeResMgr->m_textureRenderResources.find(texID) == texRendeResMgr->m_textureRenderResources.end())
	{
		auto imgPath = g_system_container.m_configSystem->m_imageResourcePath / (texID + ".png");
		g_system_container.m_resourceSystem->loadImageResource(imgPath);
	}

	if (texRendeResMgr->m_textureRenderResources[texID].m_state == eTextureRenderResouceState::UNINITIALIZED)
	{
		texRendeResMgr->setupTextureRenderResources(texID);
	}

	m_vTextureParameterValues[key] = textureInfo;
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

void icpMaterialInstance::MemCopyToBuffer(void* dst, void* src, size_t size)
{
	memcpy(dst, src, size);
}

void* icpMaterialInstance::CheckMaterialDataCache()
{
	switch (m_shadingModel)
	{
	case eMaterialShadingModel::PBR_LIT:
		{
		FillPBRDataCache();
		return &m_pbrDataCache;
		}
	case eMaterialShadingModel::UNLIT:
		{
		break;
		}
	default:
		{
		break;
		}
	}
}

void icpMaterialInstance::FillPBRDataCache()
{
	m_pbrDataCache.baseColorFactor = m_vVectorParameterValues.contains("baseColorFactor") ? m_vVectorParameterValues["baseColorFactor"].m_vValue : m_pbrDataCache.baseColorFactor;
	m_pbrDataCache.emissiveFactor = m_vVectorParameterValues.contains("emissiveFactor") ? m_vVectorParameterValues["emissiveFactor"].m_vValue : m_pbrDataCache.emissiveFactor;
	//m_pbrDataCache.diffuseFactor = m_vVectorParameterValues.contains("diffuseFactor") ? m_vScalarParameterValues["baseColorFactor"].m_fValue : m_pbrDataCache.baseColorFactor;
	//m_pbrDataCache.specularFactor = m_vVectorParameterValues.contains("specularFactor") ? m_vScalarParameterValues["baseColorFactor"].m_fValue : m_pbrDataCache.baseColorFactor;
	//m_pbrDataCache.workflow = m_vScalarParameterValues.contains("baseColorFactor") ? m_vScalarParameterValues["baseColorFactor"].m_fValue : m_pbrDataCache.baseColorFactor;
	m_pbrDataCache.colorTextureSet = m_vTextureParameterValues.contains("baseColorTexture") ? 1.f : m_pbrDataCache.colorTextureSet;
	m_pbrDataCache.PhysicalDescriptorTextureSet = m_vTextureParameterValues.contains("metallicRoughnessTexture") ? 1.f : m_pbrDataCache.PhysicalDescriptorTextureSet;
	m_pbrDataCache.metallicTextureSet = m_vTextureParameterValues.contains("metallicTextureSet") ? 1.f : m_pbrDataCache.metallicTextureSet;
	m_pbrDataCache.roughnessTextureSet = m_vTextureParameterValues.contains("roughnessTextureSet") ? 1.f : m_pbrDataCache.roughnessTextureSet;
	m_pbrDataCache.normalTextureSet = m_vTextureParameterValues.contains("normalTexture") ? 1.f : m_pbrDataCache.normalTextureSet;
	m_pbrDataCache.occlusionTextureSet = m_vTextureParameterValues.contains("occlusionTexture") ? 1.f : m_pbrDataCache.occlusionTextureSet;
	m_pbrDataCache.emissiveTextureSet = m_vTextureParameterValues.contains("emissiveTexture") ? 1.f : m_pbrDataCache.emissiveTextureSet;
	m_pbrDataCache.metallicFactor = m_vScalarParameterValues.contains("metallicFactor") ? m_vScalarParameterValues["metallicFactor"].m_fValue : m_pbrDataCache.metallicFactor;
	m_pbrDataCache.roughnessFactor = m_vScalarParameterValues.contains("roughnessFactor") ? m_vScalarParameterValues["roughnessFactor"].m_fValue : m_pbrDataCache.roughnessFactor;
	m_pbrDataCache.alphaMask = m_vScalarParameterValues.contains("alphaMask") ? m_vScalarParameterValues["alphaMask"].m_fValue : m_pbrDataCache.alphaMask;
	m_pbrDataCache.alphaMaskCutoff = m_vScalarParameterValues.contains("alphaMaskCutoff") ? m_vScalarParameterValues["alphaMaskCutoff"].m_fValue : m_pbrDataCache.alphaMaskCutoff;
	//m_pbrDataCache.emissiveStrength = m_vScalarParameterValues.contains("baseColorFactor") ? m_vScalarParameterValues["baseColorFactor"].m_fValue : m_pbrDataCache.baseColorFactor;
}


INCEPTION_END_NAMESPACE