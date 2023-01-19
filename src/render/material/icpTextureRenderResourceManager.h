#pragma once

#include "../../core/icpMacros.h"
#include "../../resource/icpResourceSystem.h"
#include "../../core/icpSystemContainer.h"

#include <vulkan/vulkan.hpp>

INCEPTION_BEGIN_NAMESPACE

class icpImageResource;
class icpVulkanRHI;

enum class eTextureRenderResouceState
{
	UNINITIALIZED = 0,
	READY,
	DISCARD
};

struct icpTextureRenderResourceInfo
{
	VkImage m_texImage{ VK_NULL_HANDLE };
	VkDeviceMemory m_texBufferMem{ VK_NULL_HANDLE };
	VkImageView m_texImageView{ VK_NULL_HANDLE };
	VkSampler m_texSampler{ VK_NULL_HANDLE };
	std::shared_ptr<icpImageResource> m_texImageRes = nullptr;
	std::string m_texId;

	eTextureRenderResouceState m_state = eTextureRenderResouceState::UNINITIALIZED;
};


class icpTextureRenderResourceManager
{
public:
	icpTextureRenderResourceManager(std::shared_ptr<icpVulkanRHI> rhi);
	virtual ~icpTextureRenderResourceManager() = default;

	void setupTextureRenderResources(const std::string& texId);
	void checkAndcleanAllDiscardedRenderResources();

	void deleteTexture(const std::string& texId);
private:

	std::shared_ptr<icpVulkanRHI> m_rhi = nullptr;
	std::map<std::string, icpTextureRenderResourceInfo> m_textureRenderResurces;

};

INCEPTION_END_NAMESPACE