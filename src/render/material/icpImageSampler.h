#pragma once

#include "../../core/icpMacros.h"
#include <vulkan/vulkan.hpp>

INCEPTION_BEGIN_NAMESPACE
class icpGPUDevice;
class icpImageResource;


struct FSamplerBuilderInfo
{
	VkFilter FilterType = VkFilter::VK_FILTER_LINEAR;
	VkSamplerAddressMode AddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	float MaxSamplerAnisotropy = 1.f;
	VkBorderColor BorderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
	std::shared_ptr<icpImageResource> ImageRes;
	std::shared_ptr<icpGPUDevice> RHI = nullptr;
};

class icpSamplerBuilder
{
public:

	static VkSampler BuildSampler(const FSamplerBuilderInfo& builder_info);

private:
};


INCEPTION_END_NAMESPACE