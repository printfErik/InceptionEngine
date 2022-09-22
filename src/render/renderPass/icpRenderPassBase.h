#pragma once
#include "../../core/icpMacros.h"
#include "../icpVulkanRHI.h"

#include <vulkan/vulkan.hpp>

INCEPTION_BEGIN_NAMESPACE

enum class eRenderPass
{
	MAIN_FORWARD_PASS = 0,
	UI_PASS,
	RENDER_PASS_COUNT
};

class icpRenderPassBase
{
public:

	struct RendePassInitInfo
	{
		std::shared_ptr<icpVulkanRHI> rhi{ nullptr };
		eRenderPass passType;
		std::shared_ptr<icpRenderPassBase> dependency{ nullptr };
	};

	struct RenederPipelineInfo
	{
		VkPipeline m_pipeline;
		VkPipelineLayout m_pipelineLayout;
	};

	icpRenderPassBase() = default;
	virtual ~icpRenderPassBase() {}

	virtual void initializeRenderPass(RendePassInitInfo initInfo) = 0;
	virtual void setupPipeline() = 0;
	virtual void cleanup() = 0;
	virtual void render() = 0;


	std::vector<VkFramebuffer> m_swapChainFramebuffers;
	VkRenderPass m_renderPassObj{ VK_NULL_HANDLE };

	RenederPipelineInfo m_pipelineInfo{};

	std::shared_ptr<icpRenderPassBase> m_dependency{ nullptr };

protected:
	std::shared_ptr<icpVulkanRHI> m_rhi;
};

INCEPTION_END_NAMESPACE