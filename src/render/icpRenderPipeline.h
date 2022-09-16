#pragma once
#include "../core/icpMacros.h"
#include <vulkan/vulkan.hpp>
#include "icpPipelineBase.h"
#include "icpVulkanRHI.h"

INCEPTION_BEGIN_NAMESPACE

class icpRenderPipeline : public icpPipelineBase
{
public:
	icpRenderPipeline();
	~icpRenderPipeline() override;

	bool initialize(std::shared_ptr<icpVulkanRHI> vulkanRHI) override;
	VkShaderModule createShaderModule(const char* shaderFileName);
	void cleanup();
	
	void cleanupSwapChain();

	void createRenderPass();
	void createGraphicsPipeline();	
	void createFrameBuffers();

	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

	void render();

	void recreateSwapChain();

	VkRenderPass m_renderPass{ VK_NULL_HANDLE };
private:

	VkPipeline m_pipeline{VK_NULL_HANDLE};
	VkPipelineLayout m_pipelineLayout{ VK_NULL_HANDLE };
	

	std::vector<VkShaderModule> m_shaderModules;

	std::shared_ptr<icpVulkanRHI> m_rhi = nullptr;

	std::vector<VkFramebuffer> m_swapChainFramebuffers;

	uint32_t m_currentFrame = 0;
};

INCEPTION_END_NAMESPACE