#include "icpDescriptorSet.h"
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
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	createInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(logicDevice, &createInfo, nullptr, &layout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor set layout!");
	}

	return layout;
}

DescriptorSetBuilder::DescriptorSetBuilder(size_t size)
{
	m_DSs.resize(MAX_FRAMES_IN_FLIGHT);
	m_descriptorInfos.resize(MAX_FRAMES_IN_FLIGHT);

	for (auto& dsInfo : m_descriptorInfos)
	{
		dsInfo.resize(size);
	}

	m_descriptorWrites.resize(MAX_FRAMES_IN_FLIGHT);
	for (auto& dsWrite : m_descriptorWrites)
	{
		dsWrite.resize(size);
	}
}

DescriptorSetBuilder& DescriptorSetBuilder::SetUniformBuffer(uint16_t dstBinding,
	const std::vector<icpBufferRenderResource>& bufferRes)
{
	for (int32_t frame = 0; frame < MAX_FRAMES_IN_FLIGHT; frame++)
	{
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = bufferRes[frame].buffer;
		bufferInfo.offset = bufferRes[frame].offset;
		bufferInfo.range = bufferRes[frame].range;

		m_descriptorInfos[frame][dstBinding] = bufferInfo;

		m_descriptorWrites[frame][dstBinding].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		m_descriptorWrites[frame][dstBinding].dstBinding = dstBinding;
		m_descriptorWrites[frame][dstBinding].dstArrayElement = 0;
		m_descriptorWrites[frame][dstBinding].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		m_descriptorWrites[frame][dstBinding].descriptorCount = 1;
		m_descriptorWrites[frame][dstBinding].pBufferInfo = &std::get<VkDescriptorBufferInfo>(m_descriptorInfos[frame][dstBinding]);
	}

	return *this;
}

DescriptorSetBuilder& DescriptorSetBuilder::SetCombinedImageSampler(
	uint16_t dstBinding,
	const icpTextureRenderResourceInfo& imgInfo,
	uint32_t viewIndex)
{
	for (int32_t frame = 0; frame < MAX_FRAMES_IN_FLIGHT; frame++)
	{
		VkDescriptorImageInfo imageInfo{};

		imageInfo.sampler = imgInfo.m_texSampler;
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = imgInfo.m_texImageViews[viewIndex];

		m_descriptorInfos[frame][dstBinding] = imageInfo;

		m_descriptorWrites[frame][dstBinding].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		m_descriptorWrites[frame][dstBinding].dstBinding = dstBinding;
		m_descriptorWrites[frame][dstBinding].dstArrayElement = 0;
		m_descriptorWrites[frame][dstBinding].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		m_descriptorWrites[frame][dstBinding].descriptorCount = 1;
		m_descriptorWrites[frame][dstBinding].pImageInfo = &std::get<VkDescriptorImageInfo>(m_descriptorInfos[frame][dstBinding]);
	}

	return *this;
}

DescriptorSetBuilder& DescriptorSetBuilder::SetInputAttachment(
	uint16_t dstBinding,
	const icpTextureRenderResourceInfo& imgInfo,
	uint32_t viewIndex)
{
	for (int32_t frame = 0; frame < MAX_FRAMES_IN_FLIGHT; frame++)
	{
		VkDescriptorImageInfo imageInfo{};

		imageInfo.sampler = imgInfo.m_texSampler;
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = imgInfo.m_texImageViews[viewIndex];

		m_descriptorInfos[frame][dstBinding] = imageInfo;

		m_descriptorWrites[frame][dstBinding].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		m_descriptorWrites[frame][dstBinding].dstBinding = dstBinding;
		m_descriptorWrites[frame][dstBinding].dstArrayElement = 0;
		m_descriptorWrites[frame][dstBinding].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		m_descriptorWrites[frame][dstBinding].descriptorCount = 1;
		m_descriptorWrites[frame][dstBinding].pImageInfo = &std::get<VkDescriptorImageInfo>(m_descriptorInfos[frame][dstBinding]);
	}
		

	return *this;
}

DescriptorSetBuilder& DescriptorSetBuilder::SetStorageImage(uint16_t dstBinding, const icpTextureRenderResourceInfo& imgInfo, uint32_t viewIndex)
{
	for (int32_t frame = 0; frame < MAX_FRAMES_IN_FLIGHT; frame++)
	{
		VkDescriptorImageInfo imageInfo{};

		imageInfo.sampler = imgInfo.m_texSampler;
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		imageInfo.imageView = imgInfo.m_texImageViews[viewIndex];

		m_descriptorInfos[frame][dstBinding] = imageInfo;

		m_descriptorWrites[frame][dstBinding].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		m_descriptorWrites[frame][dstBinding].dstBinding = dstBinding;
		m_descriptorWrites[frame][dstBinding].dstArrayElement = 0;
		m_descriptorWrites[frame][dstBinding].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		m_descriptorWrites[frame][dstBinding].descriptorCount = 1;
		m_descriptorWrites[frame][dstBinding].pImageInfo = &std::get<VkDescriptorImageInfo>(m_descriptorInfos[frame][dstBinding]);
	}

	return *this;
}

std::vector<VkDescriptorSet>& DescriptorSetBuilder::Build(VkDevice logicDevice,
	VkDescriptorPool dsPool, VkDescriptorSetLayout layout)
{
	std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, layout);

	VkDescriptorSetAllocateInfo allocateInfo{};
	allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocateInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
	allocateInfo.descriptorPool = dsPool;
	allocateInfo.pSetLayouts = layouts.data();


	if (vkAllocateDescriptorSets(logicDevice, &allocateInfo, m_DSs.data()) != VK_SUCCESS)
	{
		// icp failed
		return m_DSs;
	}

	for (int32_t frame = 0; frame < MAX_FRAMES_IN_FLIGHT; frame++)
	{
		for (auto& writeDS : m_descriptorWrites[frame])
		{
			writeDS.dstSet = m_DSs[frame];
		}

		vkUpdateDescriptorSets(logicDevice, m_descriptorWrites[frame].size(),
			m_descriptorWrites[frame].data(),
			0, nullptr);
	}

	return m_DSs;
}



INCEPTION_END_NAMESPACE