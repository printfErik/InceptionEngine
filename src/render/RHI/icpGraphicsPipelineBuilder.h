#pragma once

#include "../../core/icpMacros.h"
#include "Vulkan/icpVkGPUDevice.h"
#include <vulkan/vulkan.hpp>
#include "Vulkan/icpVulkanUtility.h"


INCEPTION_BEGIN_NAMESPACE

class GraphicsPipelineBuilder
{
public:
	
    GraphicsPipelineBuilder(std::shared_ptr<icpGPUDevice> device, VkRenderPass render_pass, uint32_t sub_pass)
        : device(device), renderPass(render_pass), subpass(sub_pass)
    {
    }

    
    GraphicsPipelineBuilder& SetPipelineLayout(const std::vector<VkDescriptorSetLayout>& DSLayouts,
        uint32_t PushConstantRangeCount,
        const VkPushConstantRange& PushConstRange);
    

    GraphicsPipelineBuilder& SetVertexShader(const std::string& path);
    

    GraphicsPipelineBuilder& SetFragmentShader(const std::string& path);
   

    GraphicsPipelineBuilder& SetVertexInput(
        const std::vector<VkVertexInputBindingDescription>& bindings,
        const std::vector<VkVertexInputAttributeDescription>& attributes);
    

    GraphicsPipelineBuilder& SetInputAssembly(VkPrimitiveTopology topology);
    

    GraphicsPipelineBuilder& SetViewport(const VkViewport& vp);
    

    GraphicsPipelineBuilder& SetScissor(const VkRect2D& sc);
    

    GraphicsPipelineBuilder& SetRasterizer(
        VkPolygonMode polygonMode, VkCullModeFlags cullMode, VkFrontFace frontFace,
        VkBool32 depthBiasEnable,
        float depthBiasConstantFactor = 0.f,
        float depthBiasSlopeFactor = 0.f,
        float depthBiasClam = 0.f);
    

    GraphicsPipelineBuilder& SetMultisampling(VkSampleCountFlagBits samples);
    

    GraphicsPipelineBuilder& SetDepthStencilState(
        VkBool32 depthTestEnable, VkBool32 depthWriteEnable, 
        VkBool32 depthBoundsTestEnable, VkBool32 stencilTestEnable, VkCompareOp compOP);


    GraphicsPipelineBuilder& SetColorBlendState(
        const std::vector<VkPipelineColorBlendAttachmentState>& attachments);

    GraphicsPipelineBuilder& SetRenderPass(VkRenderPass render_pass, uint32_t sub_pass);

    VkPipeline Build(VkPipelineLayout& pipelineLayout);
    
private:
   
    std::shared_ptr<icpGPUDevice> device;
    VkPipelineCache pipelineCache;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};

    VkViewport viewport{};
    VkRect2D scissor{};
    VkPipelineViewportStateCreateInfo viewportState{};
    VkPipelineRasterizationStateCreateInfo rasterization{};
    VkPipelineMultisampleStateCreateInfo multisample{};
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    VkPipelineColorBlendStateCreateInfo colorBlendState{};

    VkPipelineShaderStageCreateInfo vertStageInfo{};
    VkPipelineShaderStageCreateInfo fragStageInfo{};
    VkShaderModule vertShaderModule{};
    VkShaderModule fragShaderModule{};
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    VkPipelineDynamicStateCreateInfo dynamicState{};

    VkRenderPass renderPass = VK_NULL_HANDLE;
    uint32_t subpass = 0;
};

INCEPTION_END_NAMESPACE