#include "icpRenderPipeline.h"
#include "icpVulkanRHI.h"
#include <vulkan/vulkan.hpp>
#include <fstream>
#include <iterator>
#include <filesystem>

INCEPTION_BEGIN_NAMESPACE

icpRenderPipeline::icpRenderPipeline()
{

}


icpRenderPipeline::~icpRenderPipeline()
{
	cleanup();
}

void icpRenderPipeline::cleanup()
{
	vkDestroyPipeline(m_rhi->m_device, m_pipeline, nullptr);

	for (auto& shader : m_shaderModules)
	{
		vkDestroyShaderModule(m_rhi->m_device, shader, nullptr);
	}
}

bool icpRenderPipeline::initialize(std::shared_ptr<icpVulkanRHI> vulkanRHI)
{
	m_rhi = vulkanRHI;

	createRenderPass();

	VkGraphicsPipelineCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

	// Shader Configuration
	VkPipelineShaderStageCreateInfo vertShader{};
	VkPipelineShaderStageCreateInfo fragShader{};

	vertShader.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShader.module = createShaderModule("E:\\InceptionEngine\\src\\shaders\\vert.spv");
	vertShader.stage = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;
	vertShader.pName = "main";

	fragShader.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShader.module = createShaderModule("E:\\InceptionEngine\\src\\shaders\\fragment.spv");
	fragShader.stage = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShader.pName = "main";

	std::array<VkPipelineShaderStageCreateInfo, 2> shaderCreateInfos{
		vertShader, fragShader
	};

	info.stageCount = 2;
	info.pStages = shaderCreateInfos.data();
	
	// Vertex Input
	VkPipelineVertexInputStateCreateInfo vertexInput{};
	vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	
	info.pVertexInputState = &vertexInput;

	// Input Assembly
	VkPipelineInputAssemblyStateCreateInfo inputAsm{};
	inputAsm.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAsm.primitiveRestartEnable = VK_FALSE;
	inputAsm.topology = VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	info.pInputAssemblyState = &inputAsm;

	// Layout
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 0;
	pipelineLayoutInfo.pushConstantRangeCount = 0;

	if (vkCreatePipelineLayout(m_rhi->m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create pipeline layout!");
	}

	info.layout = m_pipelineLayout;

	// Viewport and Scissor
	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	
	VkViewport viewport;
	viewport.x = 0.f;
	viewport.y = 0.f;
	viewport.width = m_rhi->m_swapChainExtent.width;
	viewport.height = m_rhi->m_swapChainExtent.height;
	viewport.minDepth = 0.f;
	viewport.maxDepth = 1.f;

	viewportState.pViewports = &viewport;
	viewportState.viewportCount = 1;

	VkRect2D scissor;
	scissor.extent = m_rhi->m_swapChainExtent;
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
	rastInfo.frontFace = VkFrontFace::VK_FRONT_FACE_CLOCKWISE;
	rastInfo.polygonMode = VkPolygonMode::VK_POLYGON_MODE_FILL;

	info.pRasterizationState = &rastInfo;

	// MultiSample
	VkPipelineMultisampleStateCreateInfo multiSampleState{};
	multiSampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multiSampleState.sampleShadingEnable = VK_FALSE;
	multiSampleState.rasterizationSamples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;

	info.pMultisampleState = &multiSampleState;

	// Depth and Stencil
	info.pDepthStencilState = VK_NULL_HANDLE;

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
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	info.pDynamicState = &dynamicState;

	// RenderPass
	info.renderPass = m_renderPass;
	info.subpass = 0;

	info.basePipelineHandle = VK_NULL_HANDLE;

	if (vkCreateGraphicsPipelines(vulkanRHI->m_device, VK_NULL_HANDLE, 1, &info, VK_NULL_HANDLE, &m_pipeline) != VK_SUCCESS)
	{
		throw std::runtime_error("create renderPipeline failed");
	}

	vkDestroyShaderModule(m_rhi->m_device, vertShader.module, nullptr);
	vkDestroyShaderModule(m_rhi->m_device, fragShader.module, nullptr);

	return true;
}

VkShaderModule icpRenderPipeline::createShaderModule(const char* shaderFileName)
{
	std::ifstream inFile(shaderFileName, std::ios::binary | std::ios::ate);
	size_t fileSize = (size_t)inFile.tellg();
	std::vector<char> content(fileSize);

	inFile.seekg(0);
	inFile.read(content.data(), fileSize);

	inFile.close();

	VkShaderModuleCreateInfo creatInfo{};
	creatInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	creatInfo.pCode = reinterpret_cast<const uint32_t*>(content.data());
	creatInfo.codeSize = content.size();

	VkShaderModule shader{ VK_NULL_HANDLE };
	if (vkCreateShaderModule(m_rhi->m_device, &creatInfo, nullptr, &shader) != VK_SUCCESS)
	{
		throw std::runtime_error("create shader module failed");
	}

	m_shaderModules.push_back(shader);
	return m_shaderModules.back();
}

void icpRenderPipeline::createRenderPass()
{
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = m_rhi->m_swapChainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;

	if (vkCreateRenderPass(m_rhi->m_device, &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS) {
		throw std::runtime_error("failed to create render pass!");
	}
}


INCEPTION_END_NAMESPACE