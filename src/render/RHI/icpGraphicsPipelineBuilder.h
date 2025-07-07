#pragma once

#include "../../core/icpMacros.h"
#include "Vulkan/icpVkGPUDevice.h"
#include <vulkan/vulkan.hpp>


INCEPTION_BEGIN_NAMESPACE

class icpGraphicsPipelineBuilder
{
public:
	icpGraphicsPipelineBuilder(icpGPUDevice device)
	{

	}

private:
    VkDevice device;
    VkPipelineCache pipelineCache;

    VkPipelineVertexInputStateCreateInfo vertexInput{};
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    VkPipelineViewportStateCreateInfo viewportState{};
    VkPipelineRasterizationStateCreateInfo rasterization{};
    VkPipelineMultisampleStateCreateInfo multisample{};
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};

    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    VkPipelineDynamicStateCreateInfo dynamicState{};
    bool useDynamic = false;

    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkRenderPass renderPass = VK_NULL_HANDLE;
    uint32_t subpass = 0;
};

INCEPTION_END_NAMESPACE