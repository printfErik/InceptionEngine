#pragma once

#include "../../core/icpMacros.h"
#include "Vulkan/icpVkGPUDevice.h"
#include <vulkan/vulkan.hpp>


INCEPTION_BEGIN_NAMESPACE

class ComputePipelineBuilder
{
public:

    ComputePipelineBuilder() = delete;

    ComputePipelineBuilder(std::shared_ptr<icpGPUDevice> device)
        : device(device)
    {
    }

    ComputePipelineBuilder& SetPipelineLayout(const std::vector<VkDescriptorSetLayout>& DSLayouts,
        uint32_t PushConstantRangeCount,
        const VkPushConstantRange& PushConstRange);

    ComputePipelineBuilder& SetComputeShader(const std::string& path);

    VkPipeline Build(VkPipelineLayout& pipelineLayout);

private:

    std::shared_ptr<icpGPUDevice> device = nullptr;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};

    VkPipelineShaderStageCreateInfo ComputeStageInfo{};
    VkShaderModule ComputeShaderModule{};
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

};

INCEPTION_END_NAMESPACE