#pragma once
#include <variant>

#include "../../core/icpMacros.h"

#include <vulkan/vulkan.h>
#include "../material/icpTextureRenderResourceManager.h"
#include "icpGPUBuffer.h"

INCEPTION_BEGIN_NAMESPACE


class WriteDescriptorSetBuilder
{
public:
	WriteDescriptorSetBuilder() = default;

	WriteDescriptorSetBuilder& SetUniformBuffer(
		uint16_t dstBinding,
		const icpBufferRenderResource& bufferRes,
		uint64_t _range,
		uint64_t _offset = 0);

	WriteDescriptorSetBuilder& SetCombinedImageSampler(uint16_t binding,
		const icpTextureRenderResourceInfo& imgInfos);

	WriteDescriptorSetBuilder& SetInputAttachment(uint16_t binding,
		const icpTextureRenderResourceInfo& inputAttachmentInfos);

	std::vector<VkWriteDescriptorSet> WriteDSs;
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