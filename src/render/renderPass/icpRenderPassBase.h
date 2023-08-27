#pragma once
#include "../../core/icpMacros.h"
#include "../RHI/Vulkan/icpVkGPUDevice.h"

#include <vulkan/vulkan.hpp>

INCEPTION_BEGIN_NAMESPACE
enum class eRenderPass;
class icpEditorUI;

class icpRenderPassBase
{
public:

	struct RenderPassInitInfo
	{
		std::shared_ptr<icpVkGPUDevice> rhi{ nullptr };
		eRenderPass passType;
		std::shared_ptr<icpRenderPassBase> dependency{ nullptr };
		std::shared_ptr<icpEditorUI> editorUi{ nullptr };
	};

	struct RenderPipelineInfo
	{
		VkPipeline m_pipeline;
		VkPipelineLayout m_pipelineLayout;
	};


	icpRenderPassBase() = default;
	virtual ~icpRenderPassBase() {}

	virtual void initializeRenderPass(RenderPassInitInfo initInfo) = 0;
	virtual void setupPipeline() = 0;
	virtual void cleanup() = 0;
	virtual void render(uint32_t frameBufferIndex, uint32_t currentFrame, VkResult acquireImageResult, VkSubmitInfo& info) = 0;
	virtual void recreateSwapChain() = 0;
	virtual void createDescriptorSetLayouts() = 0;
	virtual void allocateDescriptorSets() = 0;

	std::vector<VkFramebuffer> m_swapChainFramebuffers;
	VkRenderPass m_renderPassObj{ VK_NULL_HANDLE };

	RenderPipelineInfo m_pipelineInfo{};
	std::vector<VkDescriptorSetLayout> m_DSLayouts;

	std::shared_ptr<icpRenderPassBase> m_dependency{ nullptr };

protected:
	std::shared_ptr<icpVkGPUDevice> m_rhi;
};

INCEPTION_END_NAMESPACE