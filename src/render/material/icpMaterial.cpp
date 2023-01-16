#include "icpMaterial.h"
#include "../icpVulkanRHI.h"
#include "../icpVulkanUtility.h"
#include "../icpRenderSystem.h"
#include "../../core/icpSystemContainer.h"
#include "../../resource/icpResourceSystem.h"
#include "../../core/icpLogSystem.h"
#include "../../mesh/icpMeshResource.h"
#include "../icpImageResource.h"

INCEPTION_BEGIN_NAMESPACE


void icpOneTextureMaterial::createTextureImages()
{
	// todo: remove all dynamic_cast
	auto vulkanRHI = dynamic_pointer_cast<icpVulkanRHI>(g_system_container.m_renderSystem->m_rhi);

	if (!m_imgRes)
	{
		m_imgRes = std::dynamic_pointer_cast<icpImageResource>(g_system_container.m_resourceSystem->m_resources.m_allResources[icpResourceType::TEXTURE][m_imgId]);
	}

	if (!m_imgRes)
	{
		ICP_LOG_FATAL("image resource should be valid!");
	}

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMem;

	icpVulkanUtility::createVulkanBuffer(
		m_imgRes->getImgBuffer().size(),
		VK_SHARING_MODE_EXCLUSIVE,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer,
		stagingBufferMem,
		vulkanRHI->m_device,
		vulkanRHI->m_physicalDevice
	);

	void* data;
	vkMapMemory(vulkanRHI->m_device, stagingBufferMem, 0, static_cast<uint32_t>(m_imgRes->getImgBuffer().size()), 0, &data);
	memcpy(data, m_imgRes->getImgBuffer().data(), m_imgRes->getImgBuffer().size());
	vkUnmapMemory(vulkanRHI->m_device, stagingBufferMem);

	icpVulkanUtility::createVulkanImage(
		static_cast<uint32_t>(m_imgRes->m_imgWidth),
		static_cast<uint32_t>(m_imgRes->m_height),
		static_cast<uint32_t>(m_imgRes->m_mipmapLevel),
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_textureImage,
		m_textureBufferMem,
		vulkanRHI->m_device,
		vulkanRHI->m_physicalDevice
	);

	icpVulkanUtility::transitionImageLayout(m_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<uint32_t>(m_imgRes->m_mipmapLevel), vulkanRHI->m_transferCommandPool, vulkanRHI->m_device, vulkanRHI->m_transferQueue);
	icpVulkanUtility::copyBuffer2Image(stagingBuffer, m_textureImage, static_cast<uint32_t>(m_imgRes->m_imgWidth), static_cast<uint32_t>(m_imgRes->m_height), vulkanRHI->m_transferCommandPool, vulkanRHI->m_device, vulkanRHI->m_transferQueue);
	//transitionImageLayout(m_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, static_cast<uint32_t>(imgP->m_mipmapLevel));

	vkDestroyBuffer(vulkanRHI->m_device, stagingBuffer, nullptr);
	vkFreeMemory(vulkanRHI->m_device, stagingBufferMem, nullptr);

	icpVulkanUtility::generateMipmaps(m_textureImage, VK_FORMAT_R8G8B8A8_SRGB, static_cast<uint32_t>(m_imgRes->m_imgWidth), static_cast<uint32_t>(m_imgRes->m_height), static_cast<uint32_t>(m_imgRes->m_mipmapLevel), vulkanRHI->m_graphicsCommandPool, vulkanRHI->m_device, vulkanRHI->m_graphicsQueue, vulkanRHI->m_physicalDevice);

	createTextureImageViews(m_imgRes->m_mipmapLevel);
}

void icpOneTextureMaterial::createTextureSampler()
{
	auto vulkanRHI = dynamic_pointer_cast<icpVulkanRHI>(g_system_container.m_renderSystem->m_rhi);

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

void icpOneTextureMaterial::createTextureImageViews(size_t mipmaplevel)
{
	auto vulkanRHI = dynamic_pointer_cast<icpVulkanRHI>(g_system_container.m_renderSystem->m_rhi);
	m_textureImageView = icpVulkanUtility::createImageView(m_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, mipmaplevel, vulkanRHI->m_device);
}


INCEPTION_END_NAMESPACE