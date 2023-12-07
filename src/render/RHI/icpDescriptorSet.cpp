#include "icpDescirptorSet.h"
#include "../material/icpTextureRenderResourceManager.h"
#include "Vulkan/icpVkGPUDevice.h"
#include "icpGPUBuffer.h"

INCEPTION_BEGIN_NAMESPACE

icpDescriptorSetCreation& icpDescriptorSetCreation::SetUniformBuffer(uint16_t binding, const std::vector<icpBufferRenderResourceInfo>& ub)
{
	bindings.push_back(binding);

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		std::variant<icpBufferRenderResourceInfo, icpTextureRenderResourceInfo> v = ub[i];
		resources.push_back(v);
	}

	return *this;
}

icpDescriptorSetCreation& icpDescriptorSetCreation::SetCombinedImageSampler(uint16_t binding, const std::vector<icpTextureRenderResourceInfo>& imgInfos)
{
	bindings.push_back(binding);

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		std::variant<icpBufferRenderResourceInfo, icpTextureRenderResourceInfo> v = imgInfos[i];
		resources.push_back(v);
	}

	return *this;
}

icpDescriptorSetCreation& icpDescriptorSetCreation::SetInputAttachment(uint16_t binding, const std::vector<icpTextureRenderResourceInfo>& imgInfos)
{
	bindings.push_back(binding);

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		std::variant<icpBufferRenderResourceInfo, icpTextureRenderResourceInfo> v = imgInfos[i];
		resources.push_back(v);
	}

	return *this;
}

INCEPTION_END_NAMESPACE