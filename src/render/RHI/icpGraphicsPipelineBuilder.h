#pragma once

#include "../../core/icpMacros.h"
#include "Vulkan/icpVkGPUDevice.h"
#include <vulkan/vulkan.hpp>
#include "Vulkan/icpVulkanUtility.h"


INCEPTION_BEGIN_NAMESPACE

class GraphicsPipelineBuilder
{
public:
	
    GraphicsPipelineBuilder(std::shared_ptr<icpGPUDevice> device, VkRenderPass renderPass)
        : device(device), renderPass(renderPass) 
    {
    }

    
    GraphicsPipelineBuilder& SetPipelineLayout(const std::vector<VkDescriptorSetLayout>& DSLayouts)
    {
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = DSLayouts.size(); 
        pipelineLayoutInfo.pSetLayouts = DSLayouts.data();
        pipelineLayoutInfo.pushConstantRangeCount = 0;
        return *this;
    }

    GraphicsPipelineBuilder& SetVertexShader(const std::string& path) 
    {
        vertShaderModule = icpVulkanUtility::createShaderModule(path.c_str(), device->GetLogicalDevice());
        vertStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertStageInfo.stage = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;
        vertStageInfo.module = vertShaderModule;
        vertStageInfo.pName = "main";
        shaderStages.push_back(vertStageInfo);
        return *this;
    }

    GraphicsPipelineBuilder& SetFragmentShader(const std::string& path) 
    {
        fragShaderModule = icpVulkanUtility::createShaderModule(path.c_str(), device->GetLogicalDevice());
        fragStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragStageInfo.stage = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
        fragStageInfo.module = fragShaderModule;
        fragStageInfo.pName = "main";
        shaderStages.push_back(fragStageInfo);
        return *this;
    }

    GraphicsPipelineBuilder& SetVertexInput(
        const std::vector<VkVertexInputBindingDescription>& bindings,
        const std::vector<VkVertexInputAttributeDescription>& attributes) 
    {
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindings.size());
        vertexInputInfo.pVertexBindingDescriptions = bindings.data();
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size());
        vertexInputInfo.pVertexAttributeDescriptions = attributes.data();
        return *this;
    }

    GraphicsPipelineBuilder& SetInputAssembly(VkPrimitiveTopology topology) 
    {
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = topology;
        inputAssembly.primitiveRestartEnable = VK_FALSE;
        return *this;
    }

    GraphicsPipelineBuilder& SetViewport(const VkViewport& vp) 
    {
        viewport = vp;
        return *this;
    }

    GraphicsPipelineBuilder& SetScissor(const VkRect2D& sc) 
    {
        scissor = sc;
        return *this;
    }

    GraphicsPipelineBuilder& SetRasterizer(VkPolygonMode polygonMode, VkCullModeFlags cullMode) 
    {
        rasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterization.depthClampEnable = VK_FALSE;
        rasterization.rasterizerDiscardEnable = VK_FALSE;
        rasterization.polygonMode = polygonMode;
        rasterization.lineWidth = 1.0f;
        rasterization.cullMode = cullMode;
        rasterization.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterization.depthBiasEnable = VK_FALSE;
        return *this;
    }

    GraphicsPipelineBuilder& SetMultisampling(VkSampleCountFlagBits samples) 
    {
        multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisample.sampleShadingEnable = VK_FALSE;
        multisample.rasterizationSamples = samples;
        return *this;
    }

    GraphicsPipelineBuilder& SetDepthStencilState(
        VkBool32 a, VkBool32 b, VkBool32 c, VkBool32 d, VkCompareOp compOP)
    {
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = a;
        depthStencil.depthWriteEnable = b;
        depthStencil.depthCompareOp = compOP;
        depthStencil.depthBoundsTestEnable = c;
        depthStencil.stencilTestEnable = d;
        return *this;
    }

    GraphicsPipelineBuilder& SetColorBlendState(
        const std::vector<VkPipelineColorBlendAttachmentState>& attachments) 
    {
        colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlendState.logicOpEnable = VK_FALSE;
        colorBlendState.logicOp = VK_LOGIC_OP_COPY;
        colorBlendState.blendConstants[0] = 0.0f;
        colorBlendState.blendConstants[1] = 0.0f;
        colorBlendState.blendConstants[2] = 0.0f;
        colorBlendState.blendConstants[3] = 0.0f;
        colorBlendState.attachmentCount = static_cast<uint32_t>(attachments.size());
        colorBlendState.pAttachments = attachments.data();
        return *this;
    }

    VkPipeline build() {
        // Viewport state
        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        // Dynamic state
        std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        // Assemble pipeline
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
        pipelineInfo.pStages = shaderStages.data();
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterization;
        pipelineInfo.pMultisampleState = &multisample;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlendState;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;

        VkPipeline pipeline;
        if (vkCreateGraphicsPipelines(device->GetLogicalDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create graphics pipeline");
        }
        // Cleanup shader modules
        vkDestroyShaderModule(device->GetLogicalDevice(), vertShaderModule, nullptr);
        vkDestroyShaderModule(device->GetLogicalDevice(), fragShaderModule, nullptr);

        return pipeline;
    }
    
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
    bool useDynamic = false;

    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkRenderPass renderPass = VK_NULL_HANDLE;
    uint32_t subpass = 0;
};

INCEPTION_END_NAMESPACE