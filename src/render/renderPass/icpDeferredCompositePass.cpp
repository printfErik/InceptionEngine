#include "icpDeferredCompositePass.h"

#include "icpCSMPass.h"
#include "../RHI/Vulkan/icpVulkanUtility.h"
#include "../../core/icpConfigSystem.h"
#include "../../mesh/icpMeshData.h"
#include "../icpSceneRenderer.h"
#include "../material/icpImageSampler.h"
#include "../shadow/icpShadowManager.h"

INCEPTION_BEGIN_NAMESPACE

icpDeferredCompositePass::icpDeferredCompositePass()
{
	
}

icpDeferredCompositePass::~icpDeferredCompositePass()
{
}

void icpDeferredCompositePass::Cleanup()
{
	vkDestroyPipelineLayout(m_rhi->GetLogicalDevice(), m_pipelineInfo.m_pipelineLayout, nullptr);
	vkDestroyPipeline(m_rhi->GetLogicalDevice(), m_pipelineInfo.m_pipeline, nullptr);
}

void icpDeferredCompositePass::Render(uint32_t frameBufferIndex, uint32_t currentFrame, VkResult acquireImageResult)
{
	auto mgr = m_pSceneRenderer.lock();
	RecordCommandBuffer(mgr->GetDeferredCommandBuffer(currentFrame), frameBufferIndex, currentFrame);
}

void icpDeferredCompositePass::RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t curFrame)
{
	auto renderer = m_pSceneRenderer.lock();

	vkCmdNextSubpass(commandBuffer, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineInfo.m_pipeline);

	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
		m_pipelineInfo.m_pipelineLayout, 0, 1, &m_vGBufferDSs[curFrame], 0, nullptr);

	auto sceneDs = renderer->GetSceneDescriptorSet(curFrame);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
		m_pipelineInfo.m_pipelineLayout, 1, 1, &sceneDs, 0, nullptr);

	vkCmdDraw(commandBuffer, 3, 1, 0, 0);
}

void icpDeferredCompositePass::UpdateRenderPassCB(uint32_t curFrame)
{
	
}

void icpDeferredCompositePass::InitializeRenderPass(RenderPassInitInfo initInfo)
{
	m_rhi = initInfo.device;
	m_pSceneRenderer = initInfo.sceneRenderer;

	CreateDescriptorSetLayouts();
	AllocateDescriptorSets();
	SetupPipeline();
}

void icpDeferredCompositePass::SetupPipeline()
{
	VkGraphicsPipelineCreateInfo compositePipeline{};
	compositePipeline.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

	// Shader Configuration
	VkPipelineShaderStageCreateInfo vertShader{};
	VkPipelineShaderStageCreateInfo fragShader{};

	vertShader.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	auto vertShaderPath = g_system_container.m_configSystem->m_shaderFolderPath / "DeferredComposite.vert.spv";
	vertShader.module = icpVulkanUtility::createShaderModule(vertShaderPath.generic_string().c_str(), m_rhi->GetLogicalDevice());
	vertShader.stage = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;
	vertShader.pName = "main";

	fragShader.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	auto fragShaderPath = g_system_container.m_configSystem->m_shaderFolderPath / "DeferredComposite.frag.spv";
	fragShader.module = icpVulkanUtility::createShaderModule(fragShaderPath.generic_string().c_str(), m_rhi->GetLogicalDevice());
	fragShader.stage = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShader.pName = "main";

	std::array<VkPipelineShaderStageCreateInfo, 2> shaderCreateInfos{
		vertShader, fragShader
	};

	compositePipeline.stageCount = 2;
	compositePipeline.pStages = shaderCreateInfos.data();

	// Vertex Input
	VkPipelineVertexInputStateCreateInfo vertexInput{};
	vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	compositePipeline.pVertexInputState = &vertexInput;

	// Input Assembly
	VkPipelineInputAssemblyStateCreateInfo inputAsm{};
	inputAsm.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAsm.primitiveRestartEnable = VK_FALSE;
	inputAsm.topology = VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	compositePipeline.pInputAssemblyState = &inputAsm;

	// Layout
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

	std::vector<VkDescriptorSetLayout> layouts{};
	for (auto& layoutInfo : m_DSLayouts)
	{
		layouts.push_back(layoutInfo.layout);
	}

	auto sceneRenderer = m_pSceneRenderer.lock();

	layouts.push_back(sceneRenderer->GetSceneDSLayout().layout);
	pipelineLayoutInfo.setLayoutCount = layouts.size(); // global scene ds
	pipelineLayoutInfo.pSetLayouts = layouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 0;

	if (vkCreatePipelineLayout(m_rhi->GetLogicalDevice(), &pipelineLayoutInfo, nullptr, &m_pipelineInfo.m_pipelineLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create pipeline layout!");
	}

	compositePipeline.layout = m_pipelineInfo.m_pipelineLayout;

	// Viewport and Scissor
	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;

	VkViewport viewport;
	viewport.x = 0.f;
	viewport.y = 0.f;
	viewport.width = m_rhi->GetSwapChainExtent().width;
	viewport.height = m_rhi->GetSwapChainExtent().height;
	viewport.minDepth = 0.f;
	viewport.maxDepth = 1.f;

	viewportState.pViewports = &viewport;
	viewportState.viewportCount = 1;

	VkRect2D scissor;
	scissor.extent = m_rhi->GetSwapChainExtent();
	scissor.offset = { 0,0 };

	viewportState.pScissors = &scissor;
	viewportState.scissorCount = 1;

	compositePipeline.pViewportState = &viewportState;

	// Rasterization State
	VkPipelineRasterizationStateCreateInfo rastInfo{};
	rastInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rastInfo.rasterizerDiscardEnable = VK_FALSE;
	rastInfo.depthClampEnable = VK_FALSE;
	rastInfo.depthBiasEnable = VK_FALSE;
	rastInfo.lineWidth = 1.f;
	rastInfo.cullMode = VkCullModeFlagBits::VK_CULL_MODE_NONE;
	rastInfo.frontFace = VkFrontFace::VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rastInfo.polygonMode = VkPolygonMode::VK_POLYGON_MODE_FILL;

	compositePipeline.pRasterizationState = &rastInfo;

	// MultiSample
	VkPipelineMultisampleStateCreateInfo multiSampleState{};
	multiSampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multiSampleState.sampleShadingEnable = VK_FALSE;
	multiSampleState.rasterizationSamples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;

	compositePipeline.pMultisampleState = &multiSampleState;

	// Depth and Stencil
	VkPipelineDepthStencilStateCreateInfo depthStencilState{};
	depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilState.depthTestEnable = VK_TRUE;
	depthStencilState.depthWriteEnable = VK_FALSE;
	depthStencilState.depthCompareOp = VK_COMPARE_OP_ALWAYS;
	depthStencilState.depthBoundsTestEnable = VK_FALSE;
	depthStencilState.stencilTestEnable = VK_FALSE;

	compositePipeline.pDepthStencilState = &depthStencilState;

	// Color Blend
	VkPipelineColorBlendStateCreateInfo colorBlend{};
	colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlend.logicOpEnable = VK_FALSE;
	colorBlend.logicOp = VK_LOGIC_OP_COPY;
	colorBlend.blendConstants[0] = 0.0f;
	colorBlend.blendConstants[1] = 0.0f;
	colorBlend.blendConstants[2] = 0.0f;
	colorBlend.blendConstants[3] = 0.0f;

	VkPipelineColorBlendAttachmentState attBlendState{};
	attBlendState.colorWriteMask = VkColorComponentFlagBits::VK_COLOR_COMPONENT_R_BIT
		| VkColorComponentFlagBits::VK_COLOR_COMPONENT_G_BIT
		| VkColorComponentFlagBits::VK_COLOR_COMPONENT_B_BIT
		| VkColorComponentFlagBits::VK_COLOR_COMPONENT_A_BIT;
	attBlendState.blendEnable = VK_FALSE;

	colorBlend.attachmentCount = 1;
	colorBlend.pAttachments = &attBlendState;

	compositePipeline.pColorBlendState = &colorBlend;

	// Dynamic State
	std::vector<VkDynamicState> dynamicStates =
	{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
		VK_DYNAMIC_STATE_LINE_WIDTH,
		VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY
	};

	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	compositePipeline.pDynamicState = &dynamicState;

	// RenderPass
	compositePipeline.renderPass = sceneRenderer->GetGBufferRenderPass();
	compositePipeline.subpass = 1;

	compositePipeline.basePipelineHandle = VK_NULL_HANDLE;

	if (vkCreateGraphicsPipelines(m_rhi->GetLogicalDevice(), VK_NULL_HANDLE, 1, &compositePipeline, VK_NULL_HANDLE, &m_pipelineInfo.m_pipeline) != VK_SUCCESS)
	{
		throw std::runtime_error("create renderPipeline failed");
	}

	vkDestroyShaderModule(m_rhi->GetLogicalDevice(), vertShader.module, nullptr);
	vkDestroyShaderModule(m_rhi->GetLogicalDevice(), fragShader.module, nullptr);
}

void icpDeferredCompositePass::CreateDescriptorSetLayouts()
{
	m_DSLayouts.resize(eDeferredCompositePassDSType::LAYOUT_TYPE_COUNT);
	auto logicDevice = m_rhi->GetLogicalDevice();

	// gbuffer
	{
		// set 0, binding 0 
		VkDescriptorSetLayoutBinding gbufferABinding{};
		gbufferABinding.binding = 0;
		gbufferABinding.descriptorCount = 1;
		gbufferABinding.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		gbufferABinding.pImmutableSamplers = nullptr;
		gbufferABinding.stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
		m_DSLayouts[eDeferredCompositePassDSType::GBUFFER].bindings.push_back({ VkDescriptorType::VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT });

		// set 0, binding 1 
		VkDescriptorSetLayoutBinding gbufferBBinding{};
		gbufferBBinding.binding = 1;
		gbufferBBinding.descriptorCount = 1;
		gbufferBBinding.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		gbufferBBinding.pImmutableSamplers = nullptr;
		gbufferBBinding.stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
		m_DSLayouts[eDeferredCompositePassDSType::GBUFFER].bindings.push_back({ VkDescriptorType::VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT });

		// set 0, binding 2
		VkDescriptorSetLayoutBinding gbufferCBinding{};
		gbufferCBinding.binding = 2;
		gbufferCBinding.descriptorCount = 1;
		gbufferCBinding.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		gbufferCBinding.pImmutableSamplers = nullptr;
		gbufferCBinding.stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
		m_DSLayouts[eDeferredCompositePassDSType::GBUFFER].bindings.push_back({ VkDescriptorType::VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT });

		// set 0, binding 3
		VkDescriptorSetLayoutBinding depthBinding{};
		depthBinding.binding = 3;
		depthBinding.descriptorCount = 1;
		depthBinding.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		depthBinding.pImmutableSamplers = nullptr;
		depthBinding.stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
		m_DSLayouts[eDeferredCompositePassDSType::GBUFFER].bindings.push_back({ VkDescriptorType::VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT });

		std::array<VkDescriptorSetLayoutBinding, 5> bindings{ gbufferABinding, gbufferBBinding, gbufferCBinding, depthBinding };

		VkDescriptorSetLayoutCreateInfo createInfo{};
		createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		createInfo.pBindings = bindings.data();
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

		if (vkCreateDescriptorSetLayout(logicDevice, &createInfo, nullptr, &m_DSLayouts[eDeferredCompositePassDSType::GBUFFER].layout) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create descriptor set layout!");
		}
	}

	// CSM 
	{
		// set 1, binding 0
		VkDescriptorSetLayoutBinding CascadeSplitUBO{};
		CascadeSplitUBO.binding = 0;
		CascadeSplitUBO.descriptorCount = 1;
		CascadeSplitUBO.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		CascadeSplitUBO.pImmutableSamplers = nullptr;
		CascadeSplitUBO.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		m_DSLayouts[eDeferredCompositePassDSType::CSM].bindings.push_back({ VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER });

		// set 1, binding 1
		VkDescriptorSetLayoutBinding CascadeShadowMap{};
		CascadeShadowMap.binding = 1;
		CascadeShadowMap.descriptorCount = 1;
		CascadeShadowMap.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		CascadeShadowMap.pImmutableSamplers = nullptr;
		CascadeShadowMap.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		m_DSLayouts[eDeferredCompositePassDSType::CSM].bindings.push_back({ VkDescriptorType::VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT });

		std::array<VkDescriptorSetLayoutBinding, 2> bindings{ CascadeSplitUBO, CascadeShadowMap };

		VkDescriptorSetLayoutCreateInfo createInfo{};
		createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		createInfo.pBindings = bindings.data();
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

		if (vkCreateDescriptorSetLayout(logicDevice, &createInfo, nullptr, &m_DSLayouts[eDeferredCompositePassDSType::CSM].layout) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create descriptor set layout!");
		}
	}
}

void icpDeferredCompositePass::AllocateDescriptorSets()
{
	icpDescriptorSetCreation creation{};
	creation.layoutInfo = m_DSLayouts[eDeferredCompositePassDSType::GBUFFER];

	std::vector<icpTextureRenderResourceInfo> gbufferAInfos;
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		icpTextureRenderResourceInfo texInfo{};
		texInfo.m_texSampler = VK_NULL_HANDLE;
		texInfo.m_texImageView = m_pSceneRenderer.lock()->GetGBufferAView();
		gbufferAInfos.push_back(texInfo);
	}
	creation.SetInputAttachment(0, gbufferAInfos);

	std::vector<icpTextureRenderResourceInfo> gbufferBInfos;
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		icpTextureRenderResourceInfo texInfo{};
		texInfo.m_texSampler = VK_NULL_HANDLE;
		texInfo.m_texImageView = m_pSceneRenderer.lock()->GetGBufferBView();
		gbufferBInfos.push_back(texInfo);
	}
	creation.SetInputAttachment(1, gbufferBInfos);

	std::vector<icpTextureRenderResourceInfo> gbufferCInfos;
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		icpTextureRenderResourceInfo texInfo{};
		texInfo.m_texSampler = VK_NULL_HANDLE;
		texInfo.m_texImageView = m_pSceneRenderer.lock()->GetGBufferCView();
		gbufferCInfos.push_back(texInfo);
	}
	creation.SetInputAttachment(2, gbufferCInfos);

	std::vector<icpTextureRenderResourceInfo> depthInfos;
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		icpTextureRenderResourceInfo texInfo{};
		texInfo.m_texSampler = VK_NULL_HANDLE;
		texInfo.m_texImageView = m_rhi->GetDepthImageView();
		depthInfos.push_back(texInfo);
	}
	creation.SetInputAttachment(3, depthInfos);

	m_rhi->CreateDescriptorSet(creation, m_vGBufferDSs);

	icpDescriptorSetCreation csmCreation{};
	csmCreation.layoutInfo = m_DSLayouts[eDeferredCompositePassDSType::CSM];

	std::vector<icpBufferRenderResourceInfo> bufferInfos;
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		icpBufferRenderResourceInfo bufferInfo{};
		bufferInfo.buffer = g_system_container.m_shadowSystem->m_csmCBs[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(FCascadeSMCB);
		bufferInfos.push_back(bufferInfo);
	}
	csmCreation.SetUniformBuffer(0, bufferInfos);

	FSamplerBuilderInfo buildInfo;
	buildInfo.BorderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	buildInfo.AddressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	buildInfo.RHI = m_rhi;
	auto csmSampler = icpSamplerBuilder::BuildSampler(buildInfo);

	auto csmPass = std::dynamic_pointer_cast<icpCSMPass>(m_pSceneRenderer.lock()->AccessRenderPass(eRenderPass::CSM_PASS));

	std::vector<icpTextureRenderResourceInfo> csmInfos;
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		icpTextureRenderResourceInfo texInfo{};
		texInfo.m_texSampler = csmSampler;
		texInfo.m_texImageView = csmPass->m_csmArrayViews[s_csmCascadeCount];
		depthInfos.push_back(texInfo);
	}
	csmCreation.SetCombinedImageSampler(1, depthInfos);

	m_rhi->CreateDescriptorSet(csmCreation, m_csmDSs);
}


INCEPTION_END_NAMESPACE