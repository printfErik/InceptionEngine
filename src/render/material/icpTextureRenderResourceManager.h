#pragma once

#include <mutex>

#include "../../core/icpMacros.h"
#include "../../resource/icpResourceSystem.h"
#include "../RHI/icpGPUDevice.h"

INCEPTION_BEGIN_NAMESPACE

class icpImageResource;

enum class eTextureRenderResourceState
{
	UNINITIALIZED = 0,
	LINKED,
	READY,
	DISCARD
};

struct icpTextureRenderResourceInfo
{
	std::shared_ptr<icpRHITexture> m_texture = nullptr;
	std::shared_ptr<icpRHISampler> m_sampler = nullptr;
	icpResourceState m_state = icpResourceState::UNKNOWN;
	std::shared_ptr<icpImageResource> m_texImageRes = nullptr;
	std::string m_texId;

	eTextureRenderResourceState m_stateCPU = eTextureRenderResourceState::UNINITIALIZED;
};

class icpTextureRenderResourceManager
{
public:
	icpTextureRenderResourceManager(std::shared_ptr<icpGPUDevice> rhi);
	virtual ~icpTextureRenderResourceManager() = default;

	void setupTextureRenderResources(const std::string& texId);
	void checkAndCleanAllDiscardedRenderResources();

	void deleteTexture(const std::string& texId);

	void InitializeEmptyTexture();
	bool RegisterTextureResource(const std::string& texID);

	icpTextureRenderResourceInfo& GetTextureRenderResByID(const std::string& texID);

	void UpdateManager();

	std::shared_ptr<icpGPUDevice> m_pDevice = nullptr;
	std::map<std::string, icpTextureRenderResourceInfo> m_textureRenderResources;

	std::mutex m_textureRenderResLock;
};

INCEPTION_END_NAMESPACE
