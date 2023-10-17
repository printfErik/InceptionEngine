#pragma once
#include "../core/icpMacros.h"
#include "../resource/icpResourceBase.h"
#include <vulkan.h>

INCEPTION_BEGIN_NAMESPACE
class icpSamplerResource : public icpResourceBase
{
public:
	icpSamplerResource();
	~icpSamplerResource();

	VkFilter magFilter = VkFilter::VK_FILTER_NEAREST;
	VkFilter minFilter = VkFilter::VK_FILTER_NEAREST;
	VkSamplerAddressMode addressModeU = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT;
	VkSamplerAddressMode addressModeV = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT;;
	VkSamplerAddressMode addressModeW = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT;;

};



INCEPTION_END_NAMESPACE