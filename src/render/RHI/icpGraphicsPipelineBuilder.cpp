#include "icpGraphicsPipelineBuilder.h"

INCEPTION_BEGIN_NAMESPACE

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetPipelineLayout(const std::vector<VkDescriptorSetLayout>& DSLayouts)
{
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = DSLayouts.size();
    pipelineLayoutInfo.pSetLayouts = DSLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetVertexShader(const std::string& path)
{
    vertShaderModule = icpVulkanUtility::createShaderModule(path.c_str(), device->GetLogicalDevice());
    vertStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertStageInfo.stage = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;
    vertStageInfo.module = vertShaderModule;
    vertStageInfo.pName = "main";
    shaderStages.push_back(vertStageInfo);
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetFragmentShader(const std::string& path)
{
    fragShaderModule = icpVulkanUtility::createShaderModule(path.c_str(), device->GetLogicalDevice());
    fragStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragStageInfo.stage = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
    fragStageInfo.module = fragShaderModule;
    fragStageInfo.pName = "main";
    shaderStages.push_back(fragStageInfo);
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetVertexInput(
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

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetInputAssembly(VkPrimitiveTopology topology)
{
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = topology;
    inputAssembly.primitiveRestartEnable = VK_FALSE;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetViewport(const VkViewport& vp)
{
    viewport = vp;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetScissor(const VkRect2D& sc)
{
    scissor = sc;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetRasterizer(VkPolygonMode polygonMode, VkCullModeFlags cullMode, VkFrontFace frontFace)
{
    rasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization.depthClampEnable = VK_FALSE;
    rasterization.rasterizerDiscardEnable = VK_FALSE;
    rasterization.depthBiasEnable = VK_FALSE;
    rasterization.lineWidth = 1.0f;
    rasterization.cullMode = cullMode;
    rasterization.frontFace = frontFace;
    rasterization.polygonMode = polygonMode;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetMultisampling(VkSampleCountFlagBits samples)
{
    multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample.sampleShadingEnable = VK_FALSE;
    multisample.rasterizationSamples = samples;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetDepthStencilState(
    VkBool32 depthTestEnable, VkBool32 depthWriteEnable, VkBool32 depthBoundsTestEnable, VkBool32 stencilTestEnable, VkCompareOp compOP)
{
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = depthTestEnable;
    depthStencil.depthWriteEnable = depthWriteEnable;
    depthStencil.depthCompareOp = compOP;
    depthStencil.depthBoundsTestEnable = depthBoundsTestEnable;
    depthStencil.stencilTestEnable = stencilTestEnable;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetColorBlendState(
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

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetRenderPass(VkRenderPass render_pass, uint32_t sub_pass)
{
    renderPass = render_pass;
    subpass = sub_pass;
}


VkPipeline GraphicsPipelineBuilder::Build()
{
    if (vkCreatePipelineLayout(device->GetLogicalDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create pipeline layout!");
    }

    // Viewport state
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    // Dynamic state
    std::vector<VkDynamicState> dynamicStates = {
    	VK_DYNAMIC_STATE_VIEWPORT,
    	VK_DYNAMIC_STATE_SCISSOR,
    	VK_DYNAMIC_STATE_LINE_WIDTH,
        VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY
    };
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

INCEPTION_END_NAMESPACE