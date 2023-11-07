#include "icpTextureRenderResourceManager.h"
#include "../RHI/Vulkan/icpVkGPUDevice.h"
#include "../RHI/Vulkan/icpVulkanUtility.h"
#include "../../core/icpLogSystem.h"
#include "../icpImageResource.h"
#include <vk_mem_alloc.h>

INCEPTION_BEGIN_NAMESPACE

icpTextureRenderResourceManager::icpTextureRenderResourceManager(std::shared_ptr<icpGPUDevice> rhi)
	: m_rhi(rhi)
{
	
}

void icpTextureRenderResourceManager::setupTextureRenderResources(const std::string& texId)
{
	// todo: remove all dynamic_cast
	icpTextureRenderResourceInfo info{};
	info.m_texId = texId;

	info.m_texImageRes = std::dynamic_pointer_cast<icpImageResource>(g_system_container.m_resourceSystem->GetResourceContainer()[icpResourceType::TEXTURE][texId]);

	if (!info.m_texImageRes)
	{
		ICP_LOG_FATAL("image resource should be valid!");
	}

	VkBuffer stagingBuffer;
	VmaAllocation stagingBufferAllocation;

	icpVulkanUtility::CreateGPUBuffer(
		info.m_texImageRes->getImgBuffer().size(),
		VK_SHARING_MODE_EXCLUSIVE,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		m_rhi->GetVmaAllocator(),
		stagingBufferAllocation,
		stagingBuffer
	);

	void* data;
	vmaMapMemory(m_rhi->GetVmaAllocator(), stagingBufferAllocation, &data);
	memcpy(data, info.m_texImageRes->getImgBuffer().data(), info.m_texImageRes->getImgBuffer().size());
	vmaUnmapMemory(m_rhi->GetVmaAllocator(), stagingBufferAllocation);

	icpVulkanUtility::CreateGPUImage(
		static_cast<uint32_t>(info.m_texImageRes->m_width),
		static_cast<uint32_t>(info.m_texImageRes->m_height),
		static_cast<uint32_t>(info.m_texImageRes->m_mipmapLevel),
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
		m_rhi->GetVmaAllocator(),
		info.m_texImage,
		info.m_texBufferAllocation
	);

	icpVulkanUtility::transitionImageLayout(info.m_texImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<uint32_t>(info.m_texImageRes->m_mipmapLevel), m_rhi->GetGraphicsCommandPool(), m_rhi->GetLogicalDevice(), m_rhi->GetGraphicsQueue());
	icpVulkanUtility::copyBuffer2Image(stagingBuffer, info.m_texImage, static_cast<uint32_t>(info.m_texImageRes->m_width), static_cast<uint32_t>(info.m_texImageRes->m_height), m_rhi->GetGraphicsCommandPool(), m_rhi->GetLogicalDevice(), m_rhi->GetGraphicsQueue());
		//icpVulkanUtility::transitionImageLayout(info.m_texImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1, );

	vmaDestroyBuffer(m_rhi->GetVmaAllocator(), stagingBuffer, stagingBufferAllocation);

	icpVulkanUtility::generateMipmaps(info.m_texImage, VK_FORMAT_R8G8B8A8_SRGB, static_cast<uint32_t>(info.m_texImageRes->m_width), static_cast<uint32_t>(info.m_texImageRes->m_height), static_cast<uint32_t>(info.m_texImageRes->m_mipmapLevel), m_rhi->GetGraphicsCommandPool(), m_rhi->GetLogicalDevice(), m_rhi->GetGraphicsQueue(), m_rhi->GetPhysicalDevice());

	info.m_texImageView = icpVulkanUtility::CreateGPUImageView(info.m_texImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, info.m_texImageRes->m_mipmapLevel, m_rhi->GetLogicalDevice());

	VkSamplerCreateInfo sampler{};
	sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler.magFilter = VkFilter::VK_FILTER_LINEAR;
	sampler.minFilter = VkFilter::VK_FILTER_LINEAR;

	sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

	sampler.anisotropyEnable = VK_TRUE;

	VkPhysicalDeviceProperties properties;
	vkGetPhysicalDeviceProperties(m_rhi->GetPhysicalDevice(), &properties);

	sampler.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
	sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;

	sampler.unnormalizedCoordinates = VK_FALSE;
	sampler.compareEnable = VK_FALSE;
	sampler.compareOp = VK_COMPARE_OP_ALWAYS;

	const auto imgP = info.m_texImageRes;
	sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler.mipLodBias = 0.0f;
	sampler.minLod = 0.0f;
	sampler.maxLod = static_cast<float>(imgP->m_mipmapLevel);

	if (vkCreateSampler(m_rhi->GetLogicalDevice(), &sampler, nullptr, &info.m_texSampler) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create sampler!");
	}

	info.m_state = eTextureRenderResouceState::READY;
	m_textureRenderResources[info.m_texId] = info;

}

void icpTextureRenderResourceManager::checkAndcleanAllDiscardedRenderResources()
{
	for (auto& renderRes : m_textureRenderResources)
	{
		auto& name = renderRes.first;
		auto& info = renderRes.second;

		if (info.m_state == eTextureRenderResouceState::DISCARD)
		{
			vkDestroySampler(m_rhi->GetLogicalDevice(), info.m_texSampler, nullptr);
			vkDestroyImageView(m_rhi->GetLogicalDevice(), info.m_texImageView, nullptr);
			vmaDestroyImage(m_rhi->GetVmaAllocator(), info.m_texImage, info.m_texBufferAllocation);

			m_textureRenderResources.erase(name);
		}
	}
}

void icpTextureRenderResourceManager::deleteTexture(const std::string& texId)
{
	auto& info = m_textureRenderResources[texId];

	info.m_state = eTextureRenderResouceState::DISCARD;
}

void icpTextureRenderResourceManager::InitializeEmptyTexture()
{
	icpTextureRenderResourceInfo info{};

	info.m_texId = "empty2D001";

	VkBuffer stagingBuffer;
	VmaAllocation stagingBufferAllocation;

	icpVulkanUtility::CreateGPUBuffer(
		1,
		VK_SHARING_MODE_EXCLUSIVE,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		m_rhi->GetVmaAllocator(),
		stagingBufferAllocation,
		stagingBuffer
	);

	const char defaultData[1] = { '1' };
	void* data;
	vmaMapMemory(m_rhi->GetVmaAllocator(), stagingBufferAllocation, &data);
	memcpy(data, (void*)defaultData, 1);
	vmaUnmapMemory(m_rhi->GetVmaAllocator(), stagingBufferAllocation);

	icpVulkanUtility::CreateGPUImage(
		1,
		1,
		1,
		VK_FORMAT_R8_SNORM,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
		m_rhi->GetVmaAllocator(),
		info.m_texImage,
		info.m_texBufferAllocation
	);

	icpVulkanUtility::transitionImageLayout(info.m_texImage, VK_FORMAT_R8_SNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, m_rhi->GetGraphicsCommandPool(), m_rhi->GetLogicalDevice(), m_rhi->GetGraphicsQueue());
	icpVulkanUtility::copyBuffer2Image(stagingBuffer, info.m_texImage, 1, 1, m_rhi->GetGraphicsCommandPool(), m_rhi->GetLogicalDevice(), m_rhi->GetGraphicsQueue());
	icpVulkanUtility::transitionImageLayout(info.m_texImage, VK_FORMAT_R8_SNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1, m_rhi->GetGraphicsCommandPool(), m_rhi->GetLogicalDevice(), m_rhi->GetGraphicsQueue());

	vmaDestroyBuffer(m_rhi->GetVmaAllocator(), stagingBuffer, stagingBufferAllocation);

	//icpVulkanUtility::generateMipmaps(info.m_texImage, VK_FORMAT_R8G8B8A8_SRGB, static_cast<uint32_t>(info.m_texImageRes->m_width), static_cast<uint32_t>(info.m_texImageRes->m_height), static_cast<uint32_t>(info.m_texImageRes->m_mipmapLevel), m_rhi->GetGraphicsCommandPool(), m_rhi->GetLogicalDevice(), m_rhi->GetGraphicsQueue(), m_rhi->GetPhysicalDevice());

	info.m_texImageView = icpVulkanUtility::CreateGPUImageView(info.m_texImage, VK_FORMAT_R8_SNORM, VK_IMAGE_ASPECT_COLOR_BIT, 1, m_rhi->GetLogicalDevice());

	VkSamplerCreateInfo sampler{};
	sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler.magFilter = VkFilter::VK_FILTER_LINEAR;
	sampler.minFilter = VkFilter::VK_FILTER_LINEAR;

	sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

	sampler.anisotropyEnable = VK_TRUE;

	VkPhysicalDeviceProperties properties;
	vkGetPhysicalDeviceProperties(m_rhi->GetPhysicalDevice(), &properties);

	sampler.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
	sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;

	sampler.unnormalizedCoordinates = VK_FALSE;
	sampler.compareEnable = VK_FALSE;
	sampler.compareOp = VK_COMPARE_OP_ALWAYS;

	const auto imgP = info.m_texImageRes;
	sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler.mipLodBias = 0.0f;
	sampler.minLod = 0.0f;
	sampler.maxLod = 1;

	if (vkCreateSampler(m_rhi->GetLogicalDevice(), &sampler, nullptr, &info.m_texSampler) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create sampler!");
	}

	info.m_state = eTextureRenderResouceState::READY;
	m_textureRenderResources[info.m_texId] = info;
}



INCEPTION_END_NAMESPACE