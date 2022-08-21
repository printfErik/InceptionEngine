#include "icpRenderPipeline.h"
#include "icpVulkanRHI.h"
#include "vulkan/vulkan.hpp"
#include <fstream>
#include <iterator>
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
	for (auto& shader : m_shaderModules)
	{
		m_rhi->m_device.destroyShaderModule(shader);
	}
}

bool icpRenderPipeline::initialize(std::shared_ptr<icpVulkanRHI> vulkanRHI)
{

	m_rhi = vulkanRHI;

	VkGraphicsPipelineCreateInfo info;

	// info.pVertexInputState = ;

	//info.pInputAssemblyState = ;

	std::array<VkPipelineShaderStageCreateInfo, 2> shaderCreateInfos;

	shaderCreateInfos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderCreateInfos[0].module = createShaderModule("firstTriangle.vert");
	shaderCreateInfos[0].stage = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;
	shaderCreateInfos[0].pName = "main";

	shaderCreateInfos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderCreateInfos[1].module = createShaderModule("firstTriangle.frag");
	shaderCreateInfos[1].stage = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderCreateInfos[1].pName = "main";

	info.pStages = shaderCreateInfos.data();

	//shaderCreateInfo.stage = ;

	VkPipelineViewportStateCreateInfo viewportInfo;

	VkPipelineRasterizationStateCreateInfo rastInfo;

	//info.pMultisampleState = ;

	if (vkCreateGraphicsPipelines(vulkanRHI->m_device, VK_NULL_HANDLE, 1, &info, VK_NULL_HANDLE, &m_pipeline) != VK_SUCCESS)
	{
		throw std::runtime_error("create renderPipeline failed");
	}

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