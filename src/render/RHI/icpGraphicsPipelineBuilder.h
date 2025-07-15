#pragma once

#include "../../core/icpMacros.h"
#include "Vulkan/icpVkGPUDevice.h"
#include <vulkan/vulkan.hpp>
#include "Vulkan/icpVulkanUtility.h"


INCEPTION_BEGIN_NAMESPACE

class GraphicsPipelineBuilder
{
public:
	
    GraphicsPipelineBuilder(std::shared_ptr<icpGPUDevice> device)
        : device(device)
    {
    }

    
    GraphicsPipelineBuilder& SetPipelineLayout(const std::vector<VkDescriptorSetLayout>& DSLayouts);
    

    GraphicsPipelineBuilder& SetVertexShader(const std::string& path);
    

    GraphicsPipelineBuilder& SetFragmentShader(const std::string& path);
   

    GraphicsPipelineBuilder& SetVertexInput(
        const std::vector<VkVertexInputBindingDescription>& bindings,
        const std::vector<VkVertexInputAttributeDescription>& attributes);
    

    GraphicsPipelineBuilder& SetInputAssembly(VkPrimitiveTopology topology);
    

    GraphicsPipelineBuilder& SetViewport(const VkViewport& vp);
    

    GraphicsPipelineBuilder& SetScissor(const VkRect2D& sc);
    

    GraphicsPipelineBuilder& SetRasterizer(VkPolygonMode polygonMode, VkCullModeFlags cullMode, VkFrontFace frontFace);
    

    GraphicsPipelineBuilder& SetMultisampling(VkSampleCountFlagBits samples);
    

    GraphicsPipelineBuilder& SetDepthStencilState(
        VkBool32 depthTestEnable, VkBool32 depthWriteEnable, 
        VkBool32 depthBoundsTestEnable, VkBool32 stencilTestEnable, VkCompareOp compOP);


    GraphicsPipelineBuilder& SetColorBlendState(
        const std::vector<VkPipelineColorBlendAttachmentState>& attachments);

    GraphicsPipelineBuilder& SetRenderPass(VkRenderPass render_pass, uint32_t sub_pass);

    VkPipeline Build();
    
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

    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkRenderPass renderPass = VK_NULL_HANDLE;
    uint32_t subpass = 0;
};

INCEPTION_END_NAMESPACE