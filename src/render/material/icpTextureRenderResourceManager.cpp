#include "icpTextureRenderResourceManager.h"
#include "../icpVulkanRHI.h"
#include "../icpVulkanUtility.h"
#include "../../core/icpLogSystem.h"
#include "../icpImageResource.h"

INCEPTION_BEGIN_NAMESPACE

void icpTextureRenderResourceManager::createTextureImageAndView(const std::string& texId)
{
	// todo: remove all dynamic_cast

	icpTextureRenderResourceInfo info{};
	info.m_texId = texId;

	info.m_texImageRes = std::dynamic_pointer_cast<icpImageResource>(g_system_container.m_resourceSystem->m_resources.m_allResources[icpResourceType::TEXTURE][texId]);

	if (!info.m_texImageRes)
	{
		ICP_LOG_FATAL("image resource should be valid!");
	}

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMem;

	icpVulkanUtility::createVulkanBuffer(
		info.m_texImageRes->getImgBuffer().size(),
		VK_SHARING_MODE_EXCLUSIVE,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer,
		stagingBufferMem,
		m_rhi->m_device,
		m_rhi->m_physicalDevice
	);

	void* data;
	vkMapMemory(m_rhi->m_device, stagingBufferMem, 0, static_cast<uint32_t>(info.m_texImageRes->getImgBuffer().size()), 0, &data);
	memcpy(data, info.m_texImageRes->getImgBuffer().data(), info.m_texImageRes->getImgBuffer().size());
	vkUnmapMemory(m_rhi->m_device, stagingBufferMem);

	icpVulkanUtility::createVulkanImage(
		static_cast<uint32_t>(info.m_texImageRes->m_imgWidth),
		static_cast<uint32_t>(info.m_texImageRes->m_height),
		static_cast<uint32_t>(info.m_texImageRes->m_mipmapLevel),
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		info.m_texImage,
		info.m_texBufferMem,
		m_rhi->m_device,
		m_rhi->m_physicalDevice
	);

	icpVulkanUtility::transitionImageLayout(info.m_texImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<uint32_t>(info.m_texImageRes->m_mipmapLevel), m_rhi->m_transferCommandPool, m_rhi->m_device, m_rhi->m_transferQueue);
	icpVulkanUtility::copyBuffer2Image(stagingBuffer, info.m_texImage, static_cast<uint32_t>(info.m_texImageRes->m_imgWidth), static_cast<uint32_t>(info.m_texImageRes->m_height), m_rhi->m_transferCommandPool, m_rhi->m_device, m_rhi->m_transferQueue);
		//icpVulkanUtility::transitionImageLayout(info.m_texImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1, );

	vkDestroyBuffer(m_rhi->m_device, stagingBuffer, nullptr);
	vkFreeMemory(m_rhi->m_device, stagingBufferMem, nullptr);

	icpVulkanUtility::generateMipmaps(info.m_texImage, VK_FORMAT_R8G8B8A8_SRGB, static_cast<uint32_t>(info.m_texImageRes->m_imgWidth), static_cast<uint32_t>(info.m_texImageRes->m_height), static_cast<uint32_t>(info.m_texImageRes->m_mipmapLevel), m_rhi->m_graphicsCommandPool, m_rhi->m_device, m_rhi->m_graphicsQueue, m_rhi->m_physicalDevice);

	info.m_texImageView = icpVulkanUtility::createImageView(info.m_texImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, info.m_texImageRes->m_mipmapLevel, m_rhi->m_device);

	info.m_state = eTextureRenderResouceState::READY;
	m_textureRenderResurces[info.m_texId] = info;

}

void icpTextureRenderResourceManager::createTextureSampler(const std::string& texId)
{
	VkSamplerCreateInfo sampler{};
	sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler.magFilter = VkFilter::VK_FILTER_LINEAR;
	sampler.minFilter = VkFilter::VK_FILTER_LINEAR;

	sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

	sampler.anisotropyEnable = VK_TRUE;

	VkPhysicalDeviceProperties properties;
	vkGetPhysicalDeviceProperties(vulkanRHI->m_physicalDevice, &properties);

	sampler.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
	sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;

	sampler.unnormalizedCoordinates = VK_FALSE;
	sampler.compareEnable = VK_FALSE;
	sampler.compareOp = VK_COMPARE_OP_ALWAYS;

	const auto imgP = m_imgRes;
	sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler.mipLodBias = 0.0f;
	sampler.minLod = 0.0f;
	sampler.maxLod = static_cast<float>(imgP->m_mipmapLevel);

	if (vkCreateSampler(vulkanRHI->m_device, &sampler, nullptr, &m_textureSampler) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create sampler!");
	}
}



INCEPTION_END_NAMESPACE