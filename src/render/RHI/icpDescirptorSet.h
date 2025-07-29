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
	WriteDescriptorSetBuilder(size_t size);

	WriteDescriptorSetBuilder& SetUniformBuffer(
		uint16_t dstBinding,
		const icpBufferRenderResource& bufferRes);

	WriteDescriptorSetBuilder& SetCombinedImageSampler(uint16_t dstBinding,
		const icpTextureRenderResourceInfo& imgInfo, uint32_t viewIndex = 0);

	WriteDescriptorSetBuilder& SetInputAttachment(uint16_t dstBinding,
		const icpTextureRenderResourceInfo& imgInfo, uint32_t viewIndex = 0);

	std::vector<VkWriteDescriptorSet>& Build();

private:

	std::vector<VkWriteDescriptorSet> WriteDSs;
};

class DescriptorSetLayoutBuilder
{
public:
	DescriptorSetLayoutBuilder() = default;

	DescriptorSetLayoutBuilder& SetDescriptorSetBinding(uint32_t bindIndex,
		VkDescriptorType dsType, VkShaderStageFlags stages);

	VkDescriptorSetLayout Build(VkDevice logicDevice);

	std::vector<VkDescriptorSetLayoutBinding> bindings;

	VkDescriptorSetLayout layout { VK_NULL_HANDLE };
};

INCEPTION_END_NAMESPACE