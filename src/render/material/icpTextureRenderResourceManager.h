#pragma once

#include "../../core/icpMacros.h"
#include "../../resource/icpResourceSystem.h"
#include "../../core/icpSystemContainer.h"

#include <vulkan/vulkan.hpp>

#include <vk_mem_alloc.h>
#include "../RHI/icpGPUDevice.h"

INCEPTION_BEGIN_NAMESPACE

class icpImageResource;
class icpVkGPUDevice;

enum class eTextureRenderResourceState
{
	UNINITIALIZED = 0,
	LINKED,
	READY,
	DISCARD
};

struct icpTextureRenderResourceInfo
{
	VkImage m_texImage{ VK_NULL_HANDLE };
	VmaAllocation m_texBufferAllocation{ VK_NULL_HANDLE };
	VkImageView m_texImageView{ VK_NULL_HANDLE };
	VkSampler m_texSampler{ VK_NULL_HANDLE };
	VkImageLayout m_layout = VK_IMAGE_LAYOUT_UNDEFINED;
	std::shared_ptr<icpImageResource> m_texImageRes = nullptr;
	std::string m_texId;

	eTextureRenderResourceState m_state = eTextureRenderResourceState::UNINITIALIZED;
};


class icpTextureRenderResourceManager
{
public:
	icpTextureRenderResourceManager(std::shared_ptr<icpGPUDevice> rhi);
	virtual ~icpTextureRenderResourceManager() = default;

	void setupTextureRenderResources(const std::string& texId);
	void checkAndcleanAllDiscardedRenderResources();

	void deleteTexture(const std::string& texId);

	void InitializeEmptyTexture();
	bool RegisterTextureResource(const std::string& texID);

	icpTextureRenderResourceInfo GetTextureRenderResByID(const std::string& texID);

	void UpdateManager();

	std::shared_ptr<icpGPUDevice> m_rhi = nullptr;
	std::map<std::string, icpTextureRenderResourceInfo> m_textureRenderResources;

	std::mutex m_textureRenderResLock;
};

INCEPTION_END_NAMESPACE