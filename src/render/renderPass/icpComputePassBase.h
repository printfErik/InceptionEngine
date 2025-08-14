#pragma once
#include "../../core/icpMacros.h"


INCEPTION_BEGIN_NAMESPACE

class icpComputePassBase
{
public:

	struct ComputePassInitInfo
	{
		std::shared_ptr<icpGPUDevice> device{ nullptr };
		std::weak_ptr<icpSceneRenderer> sceneRenderer;
	};

	struct RenderPipelineInfo
	{
		VkPipeline m_pipeline;
		VkPipelineLayout m_pipelineLayout;
	};


	icpComputePassBase() = default;
	virtual ~icpComputePassBase() {}

	virtual void InitializeRenderPass(ComputePassInitInfo initInfo) = 0;
	virtual void SetupPipeline() = 0;
	virtual void Cleanup() = 0;
	virtual void Dispatch(uint32_t frameBufferIndex, uint32_t currentFrame, VkResult acquireImageResult) = 0;
	
	virtual void UpdateRenderPassCB(uint32_t curFrame) = 0;
	virtual void AllocatedRenderPassDescriptorSets() = 0;


	RenderPipelineInfo m_pipelineInfo{};

	std::vector<VkDescriptorSetLayout> dsLayouts;
protected:
	std::weak_ptr<icpSceneRenderer> m_pSceneRenderer;
	std::shared_ptr<icpGPUDevice> m_pDevice = nullptr;
};

INCEPTION_END_NAMESPACE
