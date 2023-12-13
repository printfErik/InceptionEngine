#include "icpShadowPass.h"
#include "../RHI/Vulkan/icpVulkanUtility.h"
#include "../../core/icpConfigSystem.h"
#include "../../mesh/icpMeshData.h"
#include "../icpSceneRenderer.h"
#include "../light/icpLightSystem.h"

INCEPTION_BEGIN_NAMESPACE

icpShadowPass::icpShadowPass()
	: m_nShadowMapDim(4096u),
	m_fDepthBiasConstantFactor(1.5f),
	m_fDepthBiasClamp(0.f),
	m_fDepthBiasSlopeFactor(1.75f)
{
	
}

icpShadowPass::~icpShadowPass()
{
	
}


void icpShadowPass::InitializeRenderPass(RenderPassInitInfo initInfo)
{
	m_rhi = initInfo.device;
	m_pSceneRenderer = initInfo.sceneRenderer;

	CreateDescriptorSetLayouts();
	SetupPipeline();
}

void icpShadowPass::Cleanup()
{
	
}

void icpShadowPass::SetupPipeline()
{
	VkGraphicsPipelineCreateInfo shadowPipeline{};
	shadowPipeline.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

	VkPipelineShaderStageCreateInfo vertShader{};

	vertShader.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	auto vertShaderPath = g_system_container.m_configSystem->m_shaderFolderPath / "ShadowMapping.vert.spv";
	vertShader.module = icpVulkanUtility::createShaderModule(vertShaderPath.generic_string().c_str(), m_rhi->GetLogicalDevice());
	vertShader.stage = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;
	vertShader.pName = "main";

	std::array<VkPipelineShaderStageCreateInfo, 1> shaderCreateInfos{
		vertShader
	};

	shadowPipeline.stageCount = 1;
	shadowPipeline.pStages = shaderCreateInfos.data();

	// Vertex Input
	VkPipelineVertexInputStateCreateInfo vertexInput{};

	auto bindingDescription = icpVertex::getBindingDescription();
	auto attributeDescription = icpVertex::getAttributeDescription();

	vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInput.vertexBindingDescriptionCount = 1;
	vertexInput.pVertexBindingDescriptions = &bindingDescription;
	vertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescription.size());
	vertexInput.pVertexAttributeDescriptions = attributeDescription.data();

	shadowPipeline.pVertexInputState = &vertexInput;

	// Input Assembly
	VkPipelineInputAssemblyStateCreateInfo inputAsm{};
	inputAsm.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAsm.primitiveRestartEnable = VK_FALSE;
	inputAsm.topology = VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	shadowPipeline.pInputAssemblyState = &inputAsm;

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

	shadowPipeline.layout = m_pipelineInfo.m_pipelineLayout;

	// Viewport and Scissor
	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;

	VkViewport viewport;
	viewport.x = 0.f;
	viewport.y = 0.f;
	viewport.width = static_cast<float>(m_nShadowMapDim);
	viewport.height = static_cast<float>(m_nShadowMapDim);
	viewport.minDepth = 0.f;
	viewport.maxDepth = 1.f;

	viewportState.pViewports = &viewport;
	viewportState.viewportCount = 1;

	VkRect2D scissor;
	VkExtent2D actualExtent = { m_nShadowMapDim, m_nShadowMapDim };
	scissor.extent = actualExtent;
	scissor.offset = { 0,0 };

	viewportState.pScissors = &scissor;
	viewportState.scissorCount = 1;

	shadowPipeline.pViewportState = &viewportState;

	// Rsterization State
	VkPipelineRasterizationStateCreateInfo rastInfo{};
	rastInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rastInfo.rasterizerDiscardEnable = VK_FALSE;
	rastInfo.depthClampEnable = VK_FALSE;
	rastInfo.depthBiasEnable = VK_TRUE;
	rastInfo.depthBiasConstantFactor = m_fDepthBiasConstantFactor;
	rastInfo.depthBiasClamp = m_fDepthBiasClamp;
	rastInfo.depthBiasSlopeFactor = m_fDepthBiasSlopeFactor;

	rastInfo.lineWidth = 1.f;
	rastInfo.cullMode = VkCullModeFlagBits::VK_CULL_MODE_NONE;
	rastInfo.frontFace = VkFrontFace::VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rastInfo.polygonMode = VkPolygonMode::VK_POLYGON_MODE_FILL;

	shadowPipeline.pRasterizationState = &rastInfo;

	// MultiSample
	VkPipelineMultisampleStateCreateInfo multiSampleState{};
	multiSampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multiSampleState.sampleShadingEnable = VK_FALSE;
	multiSampleState.rasterizationSamples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;

	shadowPipeline.pMultisampleState = &multiSampleState;

	// Depth and Stencil
	VkPipelineDepthStencilStateCreateInfo depthStencilState{};
	depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilState.depthTestEnable = VK_TRUE;
	depthStencilState.depthWriteEnable = VK_TRUE;
	depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	depthStencilState.depthBoundsTestEnable = VK_FALSE;
	depthStencilState.stencilTestEnable = VK_FALSE;

	shadowPipeline.pDepthStencilState = &depthStencilState;

	// Color Blend
	VkPipelineColorBlendStateCreateInfo colorBlend{};
	colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlend.logicOpEnable = VK_FALSE;
	colorBlend.logicOp = VK_LOGIC_OP_COPY;
	colorBlend.blendConstants[0] = 0.0f;
	colorBlend.blendConstants[1] = 0.0f;
	colorBlend.blendConstants[2] = 0.0f;
	colorBlend.blendConstants[3] = 0.0f;

	colorBlend.attachmentCount = 0;
	colorBlend.pAttachments = VK_NULL_HANDLE;

	shadowPipeline.pColorBlendState = &colorBlend;

	// RenderPass
	shadowPipeline.renderPass = sceneRenderer->GetGBufferRenderPass();
	shadowPipeline.subpass = 0;

	shadowPipeline.basePipelineHandle = VK_NULL_HANDLE;

	if (vkCreateGraphicsPipelines(m_rhi->GetLogicalDevice(), VK_NULL_HANDLE, 1, &shadowPipeline, VK_NULL_HANDLE, &m_pipelineInfo.m_pipeline) != VK_SUCCESS)
	{
		throw std::runtime_error("create renderPipeline failed");
	}

	vkDestroyShaderModule(m_rhi->GetLogicalDevice(), vertShader.module, nullptr);
}

void icpShadowPass::CreateShadowRenderPass()
{
	
}

void icpShadowPass::CreateShadowAttachment()
{
	icpVulkanUtility::CreateGPUImage(
		m_nShadowMapDim,
		m_nShadowMapDim,
		12, // log2(4096) = 12
		6 * MAX_POINT_LIGHT_NUMBER,
		icpVulkanUtility::findDepthFormat(m_rhi->GetPhysicalDevice()),
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		m_rhi->GetVmaAllocator(),
		m_shadowCubeMapArray,
		m_shadowCubeMapArrayAllocation
	);

	m_shadowCubeMapArrayView = icpVulkanUtility::CreateGPUImageView(
		m_shadowCubeMapArray,
		VK_IMAGE_VIEW_TYPE_CUBE_ARRAY,
		icpVulkanUtility::findDepthFormat(m_rhi->GetPhysicalDevice()),
		VK_IMAGE_ASPECT_DEPTH_BIT,
		12,
		6 * MAX_POINT_LIGHT_NUMBER,
		m_rhi->GetLogicalDevice()
	);
	
}

void icpShadowPass::CreateShadowFrameBuffer()
{
	auto& imageViews = m_rhi->GetSwapChainImageViews();
	m_vShadowFrameBuffers.resize(imageViews.size());

	for (size_t i = 0; i < imageViews.size(); i++)
	{
		std::array<VkImageView, 1> attachment =
		{
			m_shadowCubeMapArrayView
		};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = m_shadowRenderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachment.size());
		framebufferInfo.pAttachments = attachment.data();
		framebufferInfo.width = m_nShadowMapDim;
		framebufferInfo.height = m_nShadowMapDim;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(m_rhi->GetLogicalDevice(), &framebufferInfo, nullptr, &m_vShadowFrameBuffers[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create framebuffer!");
		}
	}
}




INCEPTION_END_NAMESPACE