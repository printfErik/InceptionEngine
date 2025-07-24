#pragma once
#include <variant>

#include "../../core/icpMacros.h"

#include <vulkan/vulkan.h>
#include "../material/icpTextureRenderResourceManager.h"
#include "icpGPUBuffer.h"

INCEPTION_BEGIN_NAMESPACE


struct icpDescriptorSetCreation
{
	icpDescriptorSetLayoutInfo layoutInfo{};

	std::vector<std::variant<icpBufferRenderResourceInfo, icpTextureRenderResourceInfo>> resources;
	std::vector<uint16_t> bindings;

	uint32_t setIndex = 0;
	icpDescriptorSetCreation& SetUniformBuffer(uint16_t binding, 
		const std::vector<icpBufferRenderResourceInfo>& ub);
	icpDescriptorSetCreation& SetCombinedImageSampler(uint16_t binding,
		const std::vector<icpTextureRenderResourceInfo>& imgInfos);
	icpDescriptorSetCreation& SetInputAttachment(uint16_t binding,
		const std::vector<icpTextureRenderResourceInfo>& inputAttachmentInfos);
};

class DescriptorSetLayoutBuilder
{
public:
	DescriptorSetLayoutBuilder() = default;

	DescriptorSetLayoutBuilder& SetDescriptorSetBinding(uint32_t bindIndex,
		VkDescriptorType dsType, VkShaderStageFlagBits stages);

	VkDescriptorSetLayout Build(VkDevice logicDevice);

	std::vector<VkDescriptorSetLayoutBinding> bindings;

	VkDescriptorSetLayout layout { VK_NULL_HANDLE };
};

INCEPTION_END_NAMESPACE