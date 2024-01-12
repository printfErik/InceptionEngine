#include "icpShadowPass.h"
#include "../RHI/Vulkan/icpVulkanUtility.h"
#include "../../core/icpConfigSystem.h"
#include "../../core/icpLogSystem.h"
#include "../../mesh/icpMeshData.h"
#include "../../mesh/icpMeshResource.h"
#include "../icpSceneRenderer.h"
#include "../light/icpLightSystem.h"
#include "../../mesh/icpMeshRendererComponent.h"

INCEPTION_BEGIN_NAMESPACE

icpShadowPass::icpShadowPass()
	: m_nShadowMapDim(1024u),
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
	VkPipelineShaderStageCreateInfo gemoShader{};
	VkPipelineShaderStageCreateInfo fragShader{};

	vertShader.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	auto vertShaderPath = g_system_container.m_configSystem->m_shaderFolderPath / "ShadowMapping.vert.spv";
	vertShader.module = icpVulkanUtility::createShaderModule(vertShaderPath.generic_string().c_str(), m_rhi->GetLogicalDevice());
	vertShader.stage = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;
	vertShader.pName = "main";

	gemoShader.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	auto gemoShaderPath = g_system_container.m_configSystem->m_shaderFolderPath / "ShadowMapping.gemo.spv";
	gemoShader.module = icpVulkanUtility::createShaderModule(gemoShaderPath.generic_string().c_str(), m_rhi->GetLogicalDevice());
	gemoShader.stage = VkShaderStageFlagBits::VK_SHADER_STAGE_GEOMETRY_BIT;
	gemoShader.pName = "main";

	fragShader.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	auto fragShaderPath = g_system_container.m_configSystem->m_shaderFolderPath / "ShadowMapping.frag.spv";
	fragShader.module = icpVulkanUtility::createShaderModule(fragShaderPath.generic_string().c_str(), m_rhi->GetLogicalDevice());
	fragShader.stage = VkShaderStageFlagBits::VK_SHADER_STAGE_GEOMETRY_BIT;
	fragShader.pName = "main";

	std::array<VkPipelineShaderStageCreateInfo, 3> shaderCreateInfos
	{
		vertShader,
		gemoShader,
		fragShader
	};

	shadowPipeline.stageCount = 3;
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
	shadowPipeline.renderPass = m_shadowRenderPass;
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
	std::array<VkAttachmentDescription, 2> attachments{};

	// Color attachment
	attachments[0].format = VK_FORMAT_R32G32_SFLOAT;
	attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	// Depth attachment
	attachments[1].format = icpVulkanUtility::findDepthFormat(m_rhi->GetPhysicalDevice());
	attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
	VkAttachmentReference depthReference = { 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

	VkSubpassDescription subpassDescription{};
	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments = &colorReference;
	subpassDescription.pDepthStencilAttachment = &depthReference;

	// todo: figure out
	VkSubpassDependency dependency{};
	dependency.srcSubpass = 0;
	dependency.dstSubpass = VK_SUBPASS_EXTERNAL;
	dependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependency.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	VkRenderPassCreateInfo shadowPassInfo{};
	shadowPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	shadowPassInfo.attachmentCount = attachments.size();
	shadowPassInfo.pAttachments = attachments.data();
	shadowPassInfo.dependencyCount = 1;
	shadowPassInfo.pDependencies = &dependency;
	shadowPassInfo.subpassCount = 1;
	shadowPassInfo.pSubpasses = &subpassDescription;

	if (vkCreateRenderPass(m_rhi->GetLogicalDevice(), &shadowPassInfo, nullptr, &m_shadowRenderPass) != VK_SUCCESS)
	{
		ICP_LOG_FATAL("failed to create shadow render pass!");
	}
}

void icpShadowPass::CreateShadowAttachment()
{
	icpVulkanUtility::CreateGPUImage(
		m_nShadowMapDim,
		m_nShadowMapDim,
		10, // log2(1024) = 10
		6 * MAX_POINT_LIGHT_NUMBER,
		VK_FORMAT_R32G32_SFLOAT,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		m_rhi->GetVmaAllocator(),
		m_shadowCubeMapArray,
		m_shadowCubeMapArrayAllocation
	);

	m_shadowCubeMapArrayView = icpVulkanUtility::CreateGPUImageView(
		m_shadowCubeMapArray,
		VK_IMAGE_VIEW_TYPE_CUBE_ARRAY,
		VK_FORMAT_R32G32_SFLOAT,
		VK_IMAGE_ASPECT_COLOR_BIT,
		10,
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
		framebufferInfo.layers = 6 * MAX_POINT_LIGHT_NUMBER;

		if (vkCreateFramebuffer(m_rhi->GetLogicalDevice(), &framebufferInfo, nullptr, &m_vShadowFrameBuffers[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create framebuffer!");
		}
	}
}

void icpShadowPass::CreateDescriptorSetLayouts()
{
	m_DSLayouts.resize(eShadowPassDSType::LAYOUT_TYPE_COUNT);
	auto logicDevice = m_rhi->GetLogicalDevice();
	// light UBOs
	{
		// set 0, binding 0 
		VkDescriptorSetLayoutBinding allLightUBOBinding{};
		allLightUBOBinding.binding = 0;
		allLightUBOBinding.descriptorCount = 1;
		allLightUBOBinding.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		allLightUBOBinding.pImmutableSamplers = nullptr;
		allLightUBOBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

		m_DSLayouts[eShadowPassDSType::LIGHT_INFO].bindings.push_back({ VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER });

		std::array<VkDescriptorSetLayoutBinding, 1> bindings{ allLightUBOBinding };

		VkDescriptorSetLayoutCreateInfo createInfo{};
		createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		createInfo.pBindings = bindings.data();
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

		if (vkCreateDescriptorSetLayout(logicDevice, &createInfo, nullptr, &m_DSLayouts[eShadowPassDSType::LIGHT_INFO].layout) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create descriptor set layout!");
		}
	}

	// per mesh
	{
		// set 1, binding 0 
		VkDescriptorSetLayoutBinding perObjectSSBOBinding{};
		perObjectSSBOBinding.binding = 0;
		perObjectSSBOBinding.descriptorCount = 1;
		perObjectSSBOBinding.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		perObjectSSBOBinding.pImmutableSamplers = nullptr;
		perObjectSSBOBinding.stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;
		m_DSLayouts[eShadowPassDSType::PER_MESH].bindings.push_back({ VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER });

		std::array<VkDescriptorSetLayoutBinding, 1> bindings{ perObjectSSBOBinding };

		VkDescriptorSetLayoutCreateInfo createInfo{};
		createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		createInfo.pBindings = bindings.data();
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

		if (vkCreateDescriptorSetLayout(logicDevice, &createInfo, nullptr, &m_DSLayouts[eShadowPassDSType::PER_MESH].layout) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create descriptor set layout!");
		}
	}
}

void icpShadowPass::UpdateRenderPassCB(uint32_t curFrame)
{
	// 1. update mesh
	// 2. update light

	// but none of above should appear here, so leave blank;
}

void icpShadowPass::AllocateDescriptorSets()
{
	
}

void icpShadowPass::Render(uint32_t frameBufferIndex, uint32_t currentFrame, VkResult acquireImageResult)
{
	auto mgr = m_pSceneRenderer.lock();
	RecordCommandBuffer(mgr->GetMainForwardCommandBuffer(currentFrame), frameBufferIndex, currentFrame);
}

void icpShadowPass::RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t curFrame)
{
	auto mgr = m_pSceneRenderer.lock();

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineInfo.m_pipeline);

	auto sceneDS = mgr->GetSceneDescriptorSet(curFrame);
	vkCmdBindDescriptorSets(commandBuffer, VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineInfo.m_pipelineLayout, 2, 1, &sceneDS, 0, nullptr);

	std::vector<VkDeviceSize> offsets{ 0 };
	auto meshView = g_system_container.m_sceneSystem->m_registry.view<icpMeshRendererComponent>();
	for (auto& mesh : g_system_container.m_sceneSystem->m_registry.view<icpMeshRendererComponent>())
	{
		const auto& meshRender = meshView.get<icpMeshRendererComponent>(mesh);

		auto& meshResId = meshRender.m_meshResId;
		auto res = g_system_container.m_resourceSystem->GetResourceContainer()[icpResourceType::MESH][meshResId];
		auto meshRes = std::dynamic_pointer_cast<icpMeshResource>(res);

		auto vertBuf = meshRender.m_vertexBuffer;
		std::vector<VkBuffer>vertexBuffers{ vertBuf };

		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers.data(), offsets.data());
		vkCmdBindIndexBuffer(commandBuffer, meshRender.m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdBindDescriptorSets(commandBuffer, VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineInfo.m_pipelineLayout, 0, 1, &meshRender.m_perMeshDSs[curFrame], 0, nullptr);
		vkCmdSetPrimitiveTopology(commandBuffer, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(meshRes->m_meshData.m_vertexIndices.size()), 1, 0, 0, 0);
	}
}



INCEPTION_END_NAMESPACE