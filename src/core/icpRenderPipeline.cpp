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

	VkGraphicsPipelineCreateInfo info;

	// Shader Configuration
	std::array<VkPipelineShaderStageCreateInfo, 2> shaderCreateInfos;

	shaderCreateInfos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

	std::filesystem::path cwd(std::filesystem::current_path());
	shaderCreateInfos[0].module = createShaderModule("E:\\InceptionEngine\\src\\shaders\\vert.spv");
	shaderCreateInfos[0].stage = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;
	shaderCreateInfos[0].pName = "main";

	shaderCreateInfos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderCreateInfos[1].module = createShaderModule("E:\\InceptionEngine\\src\\shaders\\fragment.spv");
	shaderCreateInfos[1].stage = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderCreateInfos[1].pName = "main";

	info.pStages = shaderCreateInfos.data();

	// Vertex Input
	VkPipelineVertexInputStateCreateInfo vertexInput;
	vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	
	info.pVertexInputState = &vertexInput;

	// Input Assembly
	VkPipelineInputAssemblyStateCreateInfo inputAsm;
	inputAsm.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAsm.primitiveRestartEnable = VK_FALSE;
	inputAsm.topology = VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	info.pInputAssemblyState = &inputAsm;

	// Layout

	// Viewport and Scissor
	VkPipelineViewportStateCreateInfo viewportState;
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	
	VkViewport viewport;
	viewport.x = 0.f;
	viewport.y = 0.f;
	viewport.width = m_rhi->m_swapChainExtent.width;
	viewport.height = m_rhi->m_swapChainExtent.height;
	viewport.minDepth = 0.f;
	viewport.maxDepth = 1.f;

	viewportState.pViewports = &viewport;

	VkRect2D scissor;
	scissor.extent = m_rhi->m_swapChainExtent;
	scissor.offset = { 0,0 };
	
	viewportState.pScissors = &scissor;

	info.pViewportState = &viewportState;

	// Rsterization State
	VkPipelineRasterizationStateCreateInfo rastInfo;
	rastInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rastInfo.rasterizerDiscardEnable = VK_FALSE;
	rastInfo.depthClampEnable = VK_FALSE;
	rastInfo.depthBiasEnable = VK_FALSE;
	rastInfo.lineWidth = 1.f;
	rastInfo.cullMode = VkCullModeFlagBits::VK_CULL_MODE_NONE;
	rastInfo.polygonMode = VkPolygonMode::VK_POLYGON_MODE_FILL;

	info.pRasterizationState = &rastInfo;

	// MultiSample
	VkPipelineMultisampleStateCreateInfo multiSampleState;
	multiSampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multiSampleState.sampleShadingEnable = VK_FALSE;
	multiSampleState.rasterizationSamples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;

	info.pMultisampleState = &multiSampleState;

	// Depth and Stencil
	info.pDepthStencilState = VK_NULL_HANDLE;

	// Color Blend
	VkPipelineColorBlendStateCreateInfo colorBlend;
	colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlend.logicOpEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState attBlendState;
	attBlendState.colorWriteMask = VkColorComponentFlagBits::VK_COLOR_COMPONENT_R_BIT
		| VkColorComponentFlagBits::VK_COLOR_COMPONENT_G_BIT
		| VkColorComponentFlagBits::VK_COLOR_COMPONENT_B_BIT
		| VkColorComponentFlagBits::VK_COLOR_COMPONENT_A_BIT;
	colorBlend.pAttachments = &attBlendState;

	info.pColorBlendState = &colorBlend;


	if (vkCreateGraphicsPipelines(vulkanRHI->m_device, VK_NULL_HANDLE, 1, &info, VK_NULL_HANDLE, &m_pipeline) != VK_SUCCESS)
	{
		throw std::runtime_error("create renderPipeline failed");
	}

	return true;
}

VkShaderModule icpRenderPipeline::createShaderModule(const char* shaderFileName)
{
	std::ifstream inFile(shaderFileName, std::ios::binary | std::ios::in);
	std::vector<char> content((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
	inFile.close();

	VkShaderModuleCreateInfo creatInfo;
	creatInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	creatInfo.pCode = (uint32_t*)content.data();
	creatInfo.codeSize = content.size();

	VkShaderModule shader{ VK_NULL_HANDLE };
	if (vkCreateShaderModule(m_rhi->m_device, &creatInfo, nullptr, &shader) != VK_SUCCESS)
	{
		throw std::runtime_error("create shader module failed");
	}

	m_shaderModules.push_back(shader);
	return m_shaderModules.back();
}



INCEPTION_END_NAMESPACE