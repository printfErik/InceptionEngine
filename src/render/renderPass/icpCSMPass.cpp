#include "icpCSMPass.h"
#include "../../core/icpConfigSystem.h"
#include "../RHI/Vulkan/icpVulkanUtility.h"
#include "../../mesh/icpMeshData.h"
#include "../icpSceneRenderer.h"
#include "../shadow/icpShadowManager.h"
#include "../../core/icpLogSystem.h"

INCEPTION_BEGIN_NAMESPACE

icpCSMPass::icpCSMPass()
{

}


icpCSMPass::~icpCSMPass()
{
	
}

void icpCSMPass::CreateCSMRenderPass()
{
	std::array<VkAttachmentDescription, 1> attachments{};

	// Depth attachment
	attachments[0].format = m_rhi->GetDepthFormat();
	attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkAttachmentReference depthReference = { 0, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

	VkSubpassDescription subpassDescription{};
	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescription.colorAttachmentCount = 0;
	subpassDescription.pColorAttachments = nullptr;
	subpassDescription.pDepthStencilAttachment = &depthReference;

	std::array<VkSubpassDependency, 2> dependency;
	dependency[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency[0].dstSubpass = 0;
	dependency[0].srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	dependency[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency[0].srcAccessMask = 0;
	dependency[0].dstAccessMask =  VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependency[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependency[1].srcSubpass = 0;
	dependency[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependency[1].srcStageMask =  VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	dependency[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	dependency[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependency[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	dependency[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	VkRenderPassCreateInfo shadowPassInfo{};
	shadowPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	shadowPassInfo.attachmentCount = attachments.size();
	shadowPassInfo.pAttachments = attachments.data();
	shadowPassInfo.dependencyCount = 2;
	shadowPassInfo.pDependencies = dependency.data();
	shadowPassInfo.subpassCount = 1;
	shadowPassInfo.pSubpasses = &subpassDescription;

	if (vkCreateRenderPass(m_rhi->GetLogicalDevice(), &shadowPassInfo, nullptr, &m_shadowRenderPass) != VK_SUCCESS)
	{
		ICP_LOG_FATAL("failed to create cascade shadow map render pass!");
	}
}

void icpCSMPass::CreateCSMImageRenderResource()
{
	icpVulkanUtility::CreateGPUImage(
		s_cascadeShadowMapResolution,
		s_cascadeShadowMapResolution,
		1,
		s_csmCascadeCount,
		m_rhi->GetDepthFormat(),
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		m_rhi->GetVmaAllocator(),
		m_csmArray, m_csmArrayAllocation
	);

	m_csmArrayView = icpVulkanUtility::CreateGPUImageView(
		m_csmArray,
		VK_IMAGE_VIEW_TYPE_2D_ARRAY,
		m_rhi->GetDepthFormat(),
		VK_IMAGE_ASPECT_DEPTH_BIT,
		1,
		s_csmCascadeCount,
		m_rhi->GetLogicalDevice()
	);
}

void icpCSMPass::CreateCSMFrameBuffer()
{
	m_csmFrameBuffers.resize(m_rhi->GetSwapChainImageViews().size());

	for (size_t i = 0; i < m_csmFrameBuffers.size(); i++)
	{
		std::array<VkImageView, 1> attachment =
		{
			m_csmArrayView
		};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = m_shadowRenderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachment.size());
		framebufferInfo.pAttachments = attachment.data();
		framebufferInfo.width = s_cascadeShadowMapResolution;
		framebufferInfo.height = s_cascadeShadowMapResolution;
		framebufferInfo.layers = s_csmCascadeCount;

		if (vkCreateFramebuffer(m_rhi->GetLogicalDevice(), &framebufferInfo, nullptr, &m_csmFrameBuffers[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create csm frame buffer!");
		}
	}

}


void icpCSMPass::InitializeRenderPass(RenderPassInitInfo initInfo)
{
	m_rhi = initInfo.device;
	m_pSceneRenderer = initInfo.sceneRenderer;

	CreateDescriptorSetLayouts();
	SetupPipeline();
}


void icpCSMPass::CreateDescriptorSetLayouts()
{
	m_DSLayouts.resize(eCSMPassDSType::LAYOUT_TYPE_COUNT);
	auto logicDevice = m_rhi->GetLogicalDevice();
	// per mesh
	{
		// set 0, binding 0 
		VkDescriptorSetLayoutBinding perObjectUBOBinding{};
		perObjectUBOBinding.binding = 0;
		perObjectUBOBinding.descriptorCount = 1;
		perObjectUBOBinding.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		perObjectUBOBinding.pImmutableSamplers = nullptr;
		perObjectUBOBinding.stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;
		m_DSLayouts[eCSMPassDSType::PER_MESH].bindings.push_back({ VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER });

		std::array<VkDescriptorSetLayoutBinding, 1> bindings{ perObjectUBOBinding };

		VkDescriptorSetLayoutCreateInfo createInfo{};
		createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		createInfo.pBindings = bindings.data();
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

		if (vkCreateDescriptorSetLayout(logicDevice, &createInfo, nullptr, &m_DSLayouts[eCSMPassDSType::PER_MESH].layout) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create descriptor set layout!");
		}
	}
}


void icpCSMPass::SetupPipeline()
{
	VkGraphicsPipelineCreateInfo csmPipeline{};
	csmPipeline.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

	// Shader Configuration
	VkPipelineShaderStageCreateInfo vertShader{};

	vertShader.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	auto vertShaderPath = g_system_container.m_configSystem->m_shaderFolderPath / "CSM.vert.spv";
	vertShader.module = icpVulkanUtility::createShaderModule(vertShaderPath.generic_string().c_str(), m_rhi->GetLogicalDevice());
	vertShader.stage = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;
	vertShader.pName = "main";

	std::array<VkPipelineShaderStageCreateInfo, 1> shaderCreateInfos{
		vertShader
	};

	csmPipeline.stageCount = 1;
	csmPipeline.pStages = shaderCreateInfos.data();

	// Vertex Input
	VkPipelineVertexInputStateCreateInfo vertexInput{};

	auto bindingDescription = icpVertex::getBindingDescription();
	auto attributeDescription = icpVertex::getAttributeDescription();

	vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInput.vertexBindingDescriptionCount = 1;
	vertexInput.pVertexBindingDescriptions = &bindingDescription;
	vertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescription.size());
	vertexInput.pVertexAttributeDescriptions = attributeDescription.data();

	csmPipeline.pVertexInputState = &vertexInput;

	// Input Assembly
	VkPipelineInputAssemblyStateCreateInfo inputAsm{};
	inputAsm.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAsm.primitiveRestartEnable = VK_FALSE;
	inputAsm.topology = VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	csmPipeline.pInputAssemblyState = &inputAsm;

	// Layout
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

	std::vector<VkDescriptorSetLayout> layouts{};
	for (auto& layoutInfo : m_DSLayouts)
	{
		layouts.push_back(layoutInfo.layout);
	}

	auto sceneRenderer = m_pSceneRenderer.lock();
	layouts.push_back(sceneRenderer->GetSceneDSLayout().layout);// global scene ds

	pipelineLayoutInfo.setLayoutCount = layouts.size(); 
	pipelineLayoutInfo.pSetLayouts = layouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 0;

	if (vkCreatePipelineLayout(m_rhi->GetLogicalDevice(), &pipelineLayoutInfo, nullptr, &m_pipelineInfo.m_pipelineLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create pipeline layout!");
	}

	csmPipeline.layout = m_pipelineInfo.m_pipelineLayout;

	// Viewport and Scissor
	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;

	VkViewport viewport;
	viewport.x = 0.f;
	viewport.y = 0.f;
	viewport.width = s_cascadeShadowMapResolution;
	viewport.height = s_cascadeShadowMapResolution;
	viewport.minDepth = 0.f;
	viewport.maxDepth = 1.f;

	viewportState.pViewports = &viewport;
	viewportState.viewportCount = 1;

	VkRect2D scissor;
	VkExtent2D csmExtent = { s_cascadeShadowMapResolution, s_cascadeShadowMapResolution };
	scissor.extent = csmExtent;
	scissor.offset = { 0,0 };

	viewportState.pScissors = &scissor;
	viewportState.scissorCount = 1;

	csmPipeline.pViewportState = &viewportState;

	// Rasterization State
	VkPipelineRasterizationStateCreateInfo rastInfo{};
	rastInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rastInfo.rasterizerDiscardEnable = VK_FALSE;
	rastInfo.depthClampEnable = VK_FALSE;
	rastInfo.depthBiasEnable = VK_TRUE;

	// todo:
	rastInfo.depthBiasConstantFactor = 1.25f;	
	rastInfo.depthBiasSlopeFactor = 1.75f;     
	rastInfo.depthBiasClamp = 0.0f;

	rastInfo.lineWidth = 1.f;
	rastInfo.cullMode = VkCullModeFlagBits::VK_CULL_MODE_BACK_BIT;
	rastInfo.frontFace = VkFrontFace::VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rastInfo.polygonMode = VkPolygonMode::VK_POLYGON_MODE_FILL;

	csmPipeline.pRasterizationState = &rastInfo;

	// MultiSample
	VkPipelineMultisampleStateCreateInfo multiSampleState{};
	multiSampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multiSampleState.sampleShadingEnable = VK_FALSE;
	multiSampleState.rasterizationSamples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;

	csmPipeline.pMultisampleState = &multiSampleState;

	// Depth and Stencil
	VkPipelineDepthStencilStateCreateInfo depthStencilState{};
	depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilState.depthTestEnable = VK_TRUE;
	depthStencilState.depthWriteEnable = VK_TRUE;
	depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	depthStencilState.depthBoundsTestEnable = VK_FALSE;
	depthStencilState.stencilTestEnable = VK_FALSE;

	csmPipeline.pDepthStencilState = &depthStencilState;

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

	csmPipeline.pColorBlendState = &colorBlend;

	// RenderPass
	csmPipeline.renderPass = m_shadowRenderPass;
	csmPipeline.subpass = 0;

	csmPipeline.basePipelineHandle = VK_NULL_HANDLE;

	if (vkCreateGraphicsPipelines(m_rhi->GetLogicalDevice(), VK_NULL_HANDLE, 1, &csmPipeline, VK_NULL_HANDLE, &m_pipelineInfo.m_pipeline) != VK_SUCCESS)
	{
		throw std::runtime_error("create renderPipeline failed");
	}

	vkDestroyShaderModule(m_rhi->GetLogicalDevice(), vertShader.module, nullptr);
}

INCEPTION_END_NAMESPACE