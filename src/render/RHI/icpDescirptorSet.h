#pragma once
#include <variant>

#include "../../core/icpMacros.h"
#include <vulkan/vulkan.h>

INCEPTION_BEGIN_NAMESPACE

struct icpTextureRenderResourceInfo;

struct icpBufferRenderResourceInfo;

struct icpDescriptorSetCreation
{
	VkDescriptorSetLayout layout{VK_NULL_HANDLE};

	std::vector<std::variant<icpBufferRenderResourceInfo, icpTextureRenderResourceInfo>> resources;
	std::vector<uint16_t> bindings;

	icpDescriptorSetCreation& SetUniformBuffer(uint16_t binding, const std::vector<icpBufferRenderResourceInfo>& ub);
};

INCEPTION_END_NAMESPACE