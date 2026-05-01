#include "icpTextureRenderResourceManager.h"

#include "../../core/icpLogSystem.h"
#include "../../core/icpSystemContainer.h"
#include "../../resource/icpResourceSystem.h"
#include "../icpImageResource.h"

INCEPTION_BEGIN_NAMESPACE

icpTextureRenderResourceManager::icpTextureRenderResourceManager(std::shared_ptr<icpGPUDevice> rhi)
	: m_pDevice(rhi)
{
}

void icpTextureRenderResourceManager::setupTextureRenderResources(const std::string& texId)
{
	auto& info = m_textureRenderResources[texId];
	if (!info.m_texImageRes)
	{
		return;
	}

	icpRHITextureDesc desc{};
	desc.width = static_cast<uint32_t>(info.m_texImageRes->m_width);
	desc.height = static_cast<uint32_t>(info.m_texImageRes->m_height);
	desc.format = icpFormat::R8G8B8A8_UNORM;
	desc.usage = icpTextureUsage::SAMPLED;
	desc.initialState = icpResourceState::SHADER_RESOURCE;
	desc.debugName = info.m_texId.c_str();

	info.m_texture = m_pDevice->CreateTexture(
		desc,
		info.m_texImageRes->getImgBuffer().data(),
		info.m_texImageRes->getImgBuffer().size());
	info.m_sampler = m_pDevice->CreateSampler();
	info.m_state = icpResourceState::SHADER_RESOURCE;
	info.m_stateCPU = eTextureRenderResourceState::READY;
}

void icpTextureRenderResourceManager::checkAndCleanAllDiscardedRenderResources()
{
	std::lock_guard<std::mutex> lock_guard(m_textureRenderResLock);
	for (auto it = m_textureRenderResources.begin(); it != m_textureRenderResources.end();)
	{
		if (it->second.m_stateCPU == eTextureRenderResourceState::DISCARD)
		{
			it = m_textureRenderResources.erase(it);
		}
		else
		{
			++it;
		}
	}
}

void icpTextureRenderResourceManager::deleteTexture(const std::string& texId)
{
	std::lock_guard<std::mutex> lock_guard(m_textureRenderResLock);
	m_textureRenderResources[texId].m_stateCPU = eTextureRenderResourceState::DISCARD;
}

void icpTextureRenderResourceManager::InitializeEmptyTexture()
{
	const uint8_t whitePixel[4] = { 255, 255, 255, 255 };

	icpTextureRenderResourceInfo info{};
	info.m_texId = "empty2D001";

	icpRHITextureDesc desc{};
	desc.width = 1;
	desc.height = 1;
	desc.format = icpFormat::R8G8B8A8_UNORM;
	desc.usage = icpTextureUsage::SAMPLED;
	desc.initialState = icpResourceState::SHADER_RESOURCE;
	desc.debugName = "empty2D001";
	info.m_texture = m_pDevice->CreateTexture(desc, whitePixel, sizeof(whitePixel));
	info.m_sampler = m_pDevice->CreateSampler();
	info.m_state = icpResourceState::SHADER_RESOURCE;
	info.m_stateCPU = eTextureRenderResourceState::READY;

	std::lock_guard<std::mutex> lock_guard(m_textureRenderResLock);
	m_textureRenderResources[info.m_texId] = info;
}

bool icpTextureRenderResourceManager::RegisterTextureResource(const std::string& texID)
{
	std::lock_guard<std::mutex> lock_guard(m_textureRenderResLock);
	if (m_textureRenderResources.find(texID) != m_textureRenderResources.end())
	{
		return true;
	}

	icpTextureRenderResourceInfo info{};
	info.m_texId = texID;
	info.m_texImageRes = std::dynamic_pointer_cast<icpImageResource>(
		g_system_container.m_resourceSystem->GetResourceContainer()[icpResourceType::TEXTURE][texID]);

	if (!info.m_texImageRes)
	{
		ICP_LOG_FATAL("image resource should be valid!");
		return false;
	}

	info.m_stateCPU = eTextureRenderResourceState::LINKED;
	m_textureRenderResources[info.m_texId] = info;
	return true;
}

void icpTextureRenderResourceManager::UpdateManager()
{
	std::lock_guard<std::mutex> lock_guard(m_textureRenderResLock);
	for (auto& textureRenderRes : m_textureRenderResources)
	{
		if (textureRenderRes.second.m_stateCPU == eTextureRenderResourceState::LINKED)
		{
			setupTextureRenderResources(textureRenderRes.first);
		}
	}
}

icpTextureRenderResourceInfo& icpTextureRenderResourceManager::GetTextureRenderResByID(const std::string& texID)
{
	std::lock_guard<std::mutex> lock_guard(m_textureRenderResLock);
	if (!m_textureRenderResources.contains(texID))
	{
		return m_textureRenderResources["empty2D001"];
	}
	return m_textureRenderResources[texID];
}

INCEPTION_END_NAMESPACE
