#pragma once
#include <variant>

#include "../../core/icpMacros.h"

#include <vulkan/vulkan.h>
#include "../material/icpTextureRenderResourceManager.h"
#include "icpGPUBuffer.h"

INCEPTION_BEGIN_NAMESPACE

struct icpDescriptorSetBindingInfo
{
	VkDescriptorType type = VK_DESCRIPTOR_TYPE_MAX_ENUM;
};

struct icpDescriptorSetLayoutInfo
{
	VkDescriptorSetLayout layout{ VK_NULL_HANDLE };
	std::vector<icpDescriptorSetBindingInfo> bindings;
};

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

INCEPTION_END_NAMESPACE