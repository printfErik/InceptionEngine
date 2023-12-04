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

		auto& indicesVec = vulkanRHI->GetQueueFamilyIndicesVector();

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			icpVulkanUtility::CreateGPUBuffer(
				UBOSize,
				mode,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				vulkanRHI->GetVmaAllocator(),
				m_perMaterialUniformBufferAllocations[i],
				m_perMaterialUniformBuffers[i],
				indicesVec.size(),
				indicesVec.data()
			);
		}
	}
}

void icpMaterialInstance::AddedTextureDescriptor(const std::string& textureType, std::vector<std::vector<icpTextureRenderResourceInfo>>& imgInfosAllFrames)
{
	auto texRenderResMgr = g_system_container.m_renderSystem->GetTextureRenderResourceManager();
	std::vector<icpTextureRenderResourceInfo> imageInfos;
	icpTextureRenderResourceInfo info{};

	if (!m_vTextureParameterValues.contains(textureType))
	{
		info = texRenderResMgr->GetTextureRenderResByID("empty2D001");
	}
	else
	{
		auto& texture = m_vTextureParameterValues[textureType];
		info = texRenderResMgr->GetTextureRenderResByID(texture.m_textureID);
	}

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		imageInfos.push_back(info);
	}

	imgInfosAllFrames.push_back(imageInfos);
}

void icpMaterialInstance::AllocateDescriptorSets()
{
	auto vulkanRHI = g_system_container.m_renderSystem->GetGPUDevice();

	icpDescriptorSetCreation creation{};
	auto& layout = g_system_container.m_renderSystem->GetSceneRenderer()
		->AccessRenderPass(eRenderPass::MAIN_FORWARD_PASS)
		->m_DSLayouts[icpMainForwardPass::eMainForwardPassDSType::PER_MATERIAL];

	creation.layoutInfo = layout;

	uint64_t UBOSize = ComputeUBOSize();
	if (UBOSize > 0)
	{
		std::vector<icpBufferRenderResourceInfo> bufferInfos;
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			icpBufferRenderResourceInfo bufferInfo{};
			bufferInfo.buffer = m_perMaterialUniformBuffers[i];
			bufferInfo.offset = 0;
			bufferInfo.range = UBOSize;

			bufferInfos.push_back(bufferInfo);
		}

		creation.SetUniformBuffer(0, bufferInfos);
	}

	std::vector<std::vector<icpTextureRenderResourceInfo>> imgInfosAllFrames;

	switch (m_shadingModel)
	{
	case eMaterialShadingModel::PBR_LIT:
		{
		AddedTextureDescriptor("baseColorTexture", imgInfosAllFrames);
		AddedTextureDescriptor("metallicRoughnessTexture", imgInfosAllFrames);
		AddedTextureDescriptor("metallicTexture", imgInfosAllFrames);
		AddedTextureDescriptor("roughnessTexture", imgInfosAllFrames);
		AddedTextureDescriptor("normalTexture", imgInfosAllFrames);
		AddedTextureDescriptor("occlusionTexture", imgInfosAllFrames);
		AddedTextureDescriptor("emissiveTexture", imgInfosAllFrames);
		break;
		}
	case eMaterialShadingModel::UNLIT:
		{
		AddedTextureDescriptor("baseColorTexture", imgInfosAllFrames);
		break;
		}
	default:
		{
			break;
		}
	}

	for (uint32_t i = 0; i < imgInfosAllFrames.size(); i++)
	{
		creation.SetCombinedImageSampler(i + (UBOSize > 0 ? 1 : 0), imgInfosAllFrames[i]);
	}

	vulkanRHI->CreateDescriptorSet(creation, m_perMaterialDSs);
}

void icpMaterialSubSystem::initializeMaterialSubSystem()
{
	
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
	auto texRes = g_system_container.m_resourceSystem->FindResourceByID(icpResourceType::TEXTURE, texID);
	if (!texRes)
	{
		auto imgPath = g_system_container.m_configSystem->m_imageResourcePath / (texID + ".png");
		texRes = g_system_container.m_resourceSystem->loadImageResource(imgPath);
	}

	texRendeResMgr->RegisterTextureResource(texID);

	m_vTextureParameterValues[key] = textureInfo;
}

uint64_t icpMaterialInstance::ComputeUBOSize()
{
	switch (m_shadingModel)
	{
	case eMaterialShadingModel::PBR_LIT:
	{
		return sizeof(PBRShaderMaterial);
	}
	case eMaterialShadingModel::UNLIT:
	{
		return sizeof(UnlitShaderMaterial);
	}
	default:
	{
		return 0;
	}
	}
	
}

uint32_t icpMaterialInstance::GetSRVNumber() const
{
	return m_nSRVs;
}

void icpMaterialInstance::SetupMaterialRenderResources()
{
	if (!m_bRenderResourcesReady)
	{
		CreateUniformBuffers();
		AllocateDescriptorSets();

		m_bRenderResourcesReady = true;
	}
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


void icpMaterialSubSystem::PrepareMaterialRenderResources()
{
	for (auto material : m_vMaterialContainer)
	{
		material->SetupMaterialRenderResources();
	}
}


INCEPTION_END_NAMESPACE