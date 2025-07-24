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

DescriptorSetLayoutBuilder& DescriptorSetLayoutBuilder::SetDescriptorSetBinding(uint32_t bindIndex, VkDescriptorType dsType, VkShaderStageFlagBits stages)
{
	VkDescriptorSetLayoutBinding binding{};
	binding.binding = bindIndex;
	binding.descriptorCount = 1;
	binding.descriptorType = dsType;
	binding.pImmutableSamplers = nullptr;
	binding.stageFlags = stages;

	bindings.push_back(binding);
	return *this;
}

VkDescriptorSetLayout DescriptorSetLayoutBuilder::Build(VkDevice logicDevice)
{
	VkDescriptorSetLayoutCreateInfo createInfo{};
	createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	createInfo.pBindings = bindings.data();
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	createInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;

	if (vkCreateDescriptorSetLayout(logicDevice, &createInfo, nullptr, &layout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor set layout!");
	}

	return layout;
}



INCEPTION_END_NAMESPACE