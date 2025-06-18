#include "icpImageSampler.h"
#include "../icpImageResource.h"
#include "../RHI/icpGPUDevice.h"

INCEPTION_BEGIN_NAMESPACE

VkSampler icpSamplerBuilder::BuildSampler(const FSamplerBuilderInfo& builder_info)
{
	VkSamplerCreateInfo sampler{};
	sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler.magFilter = builder_info.FilterType;
	sampler.minFilter = builder_info.FilterType;

	sampler.addressModeU = builder_info.AddressMode;
	sampler.addressModeV = builder_info.AddressMode;
	sampler.addressModeW = builder_info.AddressMode;

	sampler.anisotropyEnable = VK_TRUE;

	sampler.maxAnisotropy = builder_info.MaxSamplerAnisotropy;
	sampler.borderColor = builder_info.BorderColor;

	sampler.unnormalizedCoordinates = VK_FALSE;
	sampler.compareEnable = VK_FALSE;
	sampler.compareOp = VK_COMPARE_OP_ALWAYS;

	const auto imgP = builder_info.ImageRes;
	sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler.mipLodBias = 0.0f;
	sampler.minLod = 0.0f;
	sampler.maxLod = imgP ? static_cast<float>(imgP->m_mipmapLevel) : 1.f;

	VkSampler retSampler{ VK_NULL_HANDLE };
	if (vkCreateSampler(builder_info.RHI->GetLogicalDevice(), &sampler, nullptr, &retSampler) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create sampler!");
	}

	return retSampler;
}


INCEPTION_END_NAMESPACE