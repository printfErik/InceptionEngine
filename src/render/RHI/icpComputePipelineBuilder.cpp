#include "icpComputePipelineBuilder.h"

#include "Vulkan/icpVulkanUtility.h"


INCEPTION_BEGIN_NAMESPACE
	ComputePipelineBuilder& ComputePipelineBuilder::SetPipelineLayout(
    const std::vector<VkDescriptorSetLayout>& DSLayouts,
    uint32_t PushConstantRangeCount,
    const VkPushConstantRange& PushConstRange)
{
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = DSLayouts.size();
    pipelineLayoutInfo.pSetLayouts = DSLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = PushConstantRangeCount;
    pipelineLayoutInfo.pPushConstantRanges = &PushConstRange;
    return *this;
}

ComputePipelineBuilder& ComputePipelineBuilder::SetComputeShader(const std::string& path)
{
    ComputeShaderModule = icpVulkanUtility::createShaderModule(path.c_str(), device->GetLogicalDevice());
    ComputeStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ComputeStageInfo.stage = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;
    ComputeStageInfo.module = ComputeShaderModule;
    ComputeStageInfo.pName = "main";
    return *this;
}

VkPipeline ComputePipelineBuilder::Build(VkPipelineLayout& pipelineLayout)
{
    if (vkCreatePipelineLayout(device->GetLogicalDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create pipeline layout!");
    }

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.stage = ComputeStageInfo;
    pipelineInfo.layout = pipelineLayout;

    VkPipeline pipeline;
    if (vkCreateComputePipelines(device->GetLogicalDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create compute pipeline");
    }

    vkDestroyShaderModule(device->GetLogicalDevice(), ComputeShaderModule, nullptr);

    return pipeline;
}



INCEPTION_END_NAMESPACE