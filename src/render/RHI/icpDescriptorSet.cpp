#include "icpDescirptorSet.h"
#include "../material/icpTextureRenderResourceManager.h"
#include "Vulkan/icpVkGPUDevice.h"
#include "icpGPUBuffer.h"


INCEPTION_BEGIN_NAMESPACE

WriteDescriptorSetBuilder::WriteDescriptorSetBuilder(size_t size)
{
	WriteDSs.resize(size);
}

WriteDescriptorSetBuilder& WriteDescriptorSetBuilder::SetUniformBuffer(
	uint16_t dstBinding,
	const icpBufferRenderResource& bufferRes)
{
	VkDescriptorBufferInfo bufferInfo{};

	bufferInfo.buffer = bufferRes.buffer;
	bufferInfo.offset = bufferRes.offset;
	bufferInfo.range = bufferRes.range;

	WriteDSs[dstBinding].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	WriteDSs[dstBinding].dstSet = VK_NULL_HANDLE;
	WriteDSs[dstBinding].dstBinding = dstBinding;
	WriteDSs[dstBinding].dstArrayElement = 0;
	WriteDSs[dstBinding].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	WriteDSs[dstBinding].descriptorCount = 1;
	WriteDSs[dstBinding].pBufferInfo = &bufferInfo;

	return *this;
}

WriteDescriptorSetBuilder& WriteDescriptorSetBuilder::SetCombinedImageSampler(
	uint16_t dstBinding,
	const icpTextureRenderResourceInfo& imgInfo,
	uint32_t viewIndex)
{
	VkDescriptorImageInfo imageInfo{};

	imageInfo.sampler = imgInfo.m_texSampler;
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView = imgInfo.m_texImageViews[viewIndex];

	WriteDSs[dstBinding].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	WriteDSs[dstBinding].dstSet = VK_NULL_HANDLE;
	WriteDSs[dstBinding].dstBinding = dstBinding;
	WriteDSs[dstBinding].dstArrayElement = 0;
	WriteDSs[dstBinding].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	WriteDSs[dstBinding].descriptorCount = 1;
	WriteDSs[dstBinding].pImageInfo = &imageInfo;

	return *this;
}

WriteDescriptorSetBuilder& WriteDescriptorSetBuilder::SetInputAttachment(
	uint16_t dstBinding,
	const icpTextureRenderResourceInfo& imgInfo,
	uint32_t viewIndex)
{
	VkDescriptorImageInfo imageInfo{};

	imageInfo.sampler = imgInfo.m_texSampler;
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView = imgInfo.m_texImageViews[viewIndex];

	WriteDSs[dstBinding].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	WriteDSs[dstBinding].dstSet = VK_NULL_HANDLE;
	WriteDSs[dstBinding].dstBinding = dstBinding;
	WriteDSs[dstBinding].dstArrayElement = 0;
	WriteDSs[dstBinding].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
	WriteDSs[dstBinding].descriptorCount = 1;
	WriteDSs[dstBinding].pImageInfo = &imageInfo;

	return *this;
}

std::vector<VkWriteDescriptorSet>& WriteDescriptorSetBuilder::Build()
{
	return WriteDSs;
}


DescriptorSetLayoutBuilder& DescriptorSetLayoutBuilder::SetDescriptorSetBinding(
	uint32_t bindIndex,
	VkDescriptorType dsType,
	VkShaderStageFlags stages)
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