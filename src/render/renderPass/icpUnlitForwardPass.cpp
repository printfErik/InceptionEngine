#include "icpUnlitForwardPass.h"
#include "icpMainForwardPass.h"
#include "../RHI/Vulkan/icpVulkanUtility.h"
#include "../../core/icpSystemContainer.h"
#include "../../core/icpConfigSystem.h"
#include "../../mesh/icpMeshData.h"
#include "../icpRenderPassManager.h"

INCEPTION_BEGIN_NAMESPACE

icpUnlitForwardPass::~icpUnlitForwardPass()
{
	
}

void icpUnlitForwardPass::InitializeRenderPass(RenderPassInitInfo initInfo)
{
	m_rhi = initInfo.device;
	m_renderPassMgr = initInfo.renderPassMgr;

	CreateDescriptorSetLayouts();
	AllocateCommandBuffers();
	CreateRenderPass();
	SetupPipeline();
	CreateFrameBuffers();
}

void icpUnlitForwardPass::CreateDescriptorSetLayouts()
{
	m_DSLayouts.resize(LAYOUT_TYPE_COUNT);
	auto logicDevice = m_rhi->GetLogicalDevice();
	// per mesh
	{
		// set 0, binding 0 
		VkDescriptorSetLayoutBinding perObjectSSBOBinding{};
		perObjectSSBOBinding.binding = 0;
		perObjectSSBOBinding.descriptorCount = 1;
		perObjectSSBOBinding.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		perObjectSSBOBinding.pImmutableSamplers = nullptr;
		perObjectSSBOBinding.stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;
		m_DSLayouts[eUnlitForwardPassDSType::PER_MESH].bindings.push_back({ VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER });

		std::array<VkDescriptorSetLayoutBinding, 1> bindings{ perObjectSSBOBinding };

		VkDescriptorSetLayoutCreateInfo createInfo{};
		createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		createInfo.pBindings = bindings.data();
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

		if (vkCreateDescriptorSetLayout(logicDevice, &createInfo, nullptr, &m_DSLayouts[eUnlitForwardPassDSType::PER_MESH].layout) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create descriptor set layout!");
		}
	}

	// per Unlit Material
	{
		// set 1, binding 0 
		VkDescriptorSetLayoutBinding perMaterialUBOBinding{};
		perMaterialUBOBinding.binding = 0;
		perMaterialUBOBinding.descriptorCount = 1;
		perMaterialUBOBinding.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		perMaterialUBOBinding.pImmutableSamplers = nullptr;
		perMaterialUBOBinding.stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
		m_DSLayouts[eUnlitForwardPassDSType::PER_MATERIAL].bindings.push_back({ VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER });

		std::vector<VkDescriptorSetLayoutBinding> bindings {perMaterialUBOBinding};

		// set 1, binding 1
		VkDescriptorSetLayoutBinding baseColorTextureSamplerLayoutBinding{};
		baseColorTextureSamplerLayoutBinding.binding = 1;
		baseColorTextureSamplerLayoutBinding.descriptorCount = 1;
		baseColorTextureSamplerLayoutBinding.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		baseColorTextureSamplerLayoutBinding.pImmutableSamplers = nullptr;
		baseColorTextureSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		m_DSLayouts[eUnlitForwardPassDSType::PER_MATERIAL].bindings.push_back({ VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER });

		bindings.push_back(baseColorTextureSamplerLayoutBinding);

		VkDescriptorSetLayoutCreateInfo createInfo{};
		createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		createInfo.pBindings = bindings.data();
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

		if (vkCreateDescriptorSetLayout(logicDevice, &createInfo, nullptr, &m_DSLayouts[eUnlitForwardPassDSType::PER_MATERIAL].layout) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create descriptor set layout!");
		}
	}
}

void icpUnlitForwardPass::SetupPipeline()
{
	VkGraphicsPipelineCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

	// Shader Configuration
	VkPipelineShaderStageCreateInfo vertShader{};
	VkPipelineShaderStageCreateInfo fragShader{};

	vertShader.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	auto vertShaderPath = g_system_container.m_configSystem->m_shaderFolderPath / "Unlit.vert.spv";
	vertShader.module = icpVulkanUtility::createShaderModule(vertShaderPath.generic_string().c_str(), m_rhi->GetLogicalDevice());
	vertShader.stage = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;
	vertShader.pName = "main";

	fragShader.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	auto fragShaderPath = g_system_container.m_configSystem->m_shaderFolderPath / "Unlit.frag.spv";
	fragShader.module = icpVulkanUtility::createShaderModule(fragShaderPath.generic_string().c_str(), m_rhi->GetLogicalDevice());
	fragShader.stage = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShader.pName = "main";

	std::array<VkPipelineShaderStageCreateInfo, 2> shaderCreateInfos{
		vertShader, fragShader
	};

	info.stageCount = 2;
	info.pStages = shaderCreateInfos.data();

	// Vertex Input
	VkPipelineVertexInputStateCreateInfo vertexInput{};

	auto bindingDescription = icpVertex::getBindingDescription();
	auto attributeDescription = icpVertex::getAttributeDescription();

	vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInput.vertexBindingDescriptionCount = 1;
	vertexInput.pVertexBindingDescriptions = &bindingDescription;
	vertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescription.size());
	vertexInput.pVertexAttributeDescriptions = attributeDescription.data();

	info.pVertexInputState = &vertexInput;

	// Input Assembly
	VkPipelineInputAssemblyStateCreateInfo inputAsm{};
	inputAsm.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAsm.primitiveRestartEnable = VK_FALSE;
	inputAsm.topology = VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

	info.pInputAssemblyState = &inputAsm;

	// Layout
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

	std::vector<VkDescriptorSetLayout> layouts{m_renderPassMgr.lock()->m_sceneDSLayout.layout};
	for (auto& layoutInfo : m_DSLayouts)
	{
		layouts.push_back(layoutInfo.layout);
	}

	pipelineLayoutInfo.pSetLayouts = layouts.data();
	pipelineLayoutInfo.setLayoutCount = layouts.size();
	pipelineLayoutInfo.pushConstantRangeCount = 0;

	if (vkCreatePipelineLayout(m_rhi->GetLogicalDevice(), &pipelineLayoutInfo, nullptr, &m_pipelineInfo.m_pipelineLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create pipeline layout!");
	}

	info.layout = m_pipelineInfo.m_pipelineLayout;

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

	info.pViewportState = &viewportState;

	// Rsterization State
	VkPipelineRasterizationStateCreateInfo rastInfo{};
	rastInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rastInfo.rasterizerDiscardEnable = VK_FALSE;
	rastInfo.depthClampEnable = VK_FALSE;
	rastInfo.depthBiasEnable = VK_FALSE;
	rastInfo.lineWidth = 1.f;
	rastInfo.cullMode = VkCullModeFlagBits::VK_CULL_MODE_BACK_BIT;
	rastInfo.frontFace = VkFrontFace::VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rastInfo.polygonMode = VkPolygonMode::VK_POLYGON_MODE_FILL;

	info.pRasterizationState = &rastInfo;

	// MultiSample
	VkPipelineMultisampleStateCreateInfo multiSampleState{};
	multiSampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multiSampleState.sampleShadingEnable = VK_FALSE;
	multiSampleState.rasterizationSamples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;

	info.pMultisampleState = &multiSampleState;

	// Depth and Stencil
	VkPipelineDepthStencilStateCreateInfo depthStencilState{};
	depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilState.depthTestEnable = VK_TRUE;
	depthStencilState.depthWriteEnable = VK_TRUE;
	depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencilState.depthBoundsTestEnable = VK_FALSE;
	depthStencilState.stencilTestEnable = VK_FALSE;

	info.pDepthStencilState = &depthStencilState;

	// Color Blend
	VkPipelineColorBlendStateCreateInfo colorBlend{};
	colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlend.logicOpEnable = VK_FALSE;
	colorBlend.logicOp = VK_LOGIC_OP_COPY;
	colorBlend.attachmentCount = 1;
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

	colorBlend.pAttachments = &attBlendState;

	info.pColorBlendState = &colorBlend;

	// Dynamic State
	std::vector<VkDynamicState> dynamicStates =
	{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
		VK_DYNAMIC_STATE_LINE_WIDTH
	};

	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	info.pDynamicState = &dynamicState;

	// RenderPass
	info.renderPass = m_renderPassObj;
	info.subpass = 0;

	info.basePipelineHandle = VK_NULL_HANDLE;

	if (vkCreateGraphicsPipelines(m_rhi->GetLogicalDevice(), VK_NULL_HANDLE, 1, &info, VK_NULL_HANDLE, &m_pipelineInfo.m_pipeline) != VK_SUCCESS)
	{
		throw std::runtime_error("create renderPipeline failed");
	}

	vkDestroyShaderModule(m_rhi->GetLogicalDevice(), vertShader.module, nullptr);
	vkDestroyShaderModule(m_rhi->GetLogicalDevice(), fragShader.module, nullptr);
}

void icpUnlitForwardPass::AllocateCommandBuffers()
{
	m_commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

	VkCommandBufferAllocateInfo gAllocInfo{};
	gAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	gAllocInfo.commandPool = m_rhi->GetGraphicsCommandPool();
	gAllocInfo.commandBufferCount = (uint32_t)m_commandBuffers.size();
	gAllocInfo.level = VkCommandBufferLevel::VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	if (vkAllocateCommandBuffers(m_rhi->GetLogicalDevice(), &gAllocInfo, m_commandBuffers.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate graphics command buffer!");
	}
}

void icpUnlitForwardPass::CreateRenderPass()
{
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = m_rhi->GetSwapChainImageFormat();
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = icpVulkanUtility::findDepthFormat(m_rhi->GetPhysicalDevice());
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	std::array<VkAttachmentDescription, 2> attachments{ colorAttachment, depthAttachment };

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(m_rhi->GetLogicalDevice(), &renderPassInfo, nullptr, &m_renderPassObj) != VK_SUCCESS) {
		throw std::runtime_error("failed to create render pass!");
	}
}

void icpUnlitForwardPass::CreateFrameBuffers()
{
	auto& imageViews = m_rhi->GetSwapChainImageViews();
	m_swapChainFramebuffers.resize(imageViews.size());

	for (size_t i = 0; i < imageViews.size(); i++)
	{
		std::array<VkImageView, 2> attachments =
		{
			imageViews[i],
			m_rhi->GetDepthImageView()
		};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = m_renderPassObj;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = m_rhi->GetSwapChainExtent().width;
		framebufferInfo.height = m_rhi->GetSwapChainExtent().height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(m_rhi->GetLogicalDevice(), &framebufferInfo, nullptr, &m_swapChainFramebuffers[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create framebuffer!");
		}
	}
}

void icpUnlitForwardPass::Cleanup()
{
	for (auto framebuffer : m_swapChainFramebuffers)
	{
		vkDestroyFramebuffer(m_rhi->GetLogicalDevice(), framebuffer, nullptr);
	}

	vkDestroyRenderPass(m_rhi->GetLogicalDevice(), m_renderPassObj, nullptr);
	vkDestroyPipelineLayout(m_rhi->GetLogicalDevice(), m_pipelineInfo.m_pipelineLayout, nullptr);
	vkDestroyPipeline(m_rhi->GetLogicalDevice(), m_pipelineInfo.m_pipeline, nullptr);
}

void icpUnlitForwardPass::Render(uint32_t frameBufferIndex, uint32_t currentFrame, VkResult acquireImageResult, VkSubmitInfo& info)
{
	
}


INCEPTION_END_NAMESPACE