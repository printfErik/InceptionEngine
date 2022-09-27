#pragma once
#include "../../core/icpMacros.h"
#include "icpRenderPassBase.h"

INCEPTION_BEGIN_NAMESPACE

class icpMainForwardPass : public icpRenderPassBase
{
public:
	icpMainForwardPass() = default;
	virtual ~icpMainForwardPass() override;

	void initializeRenderPass(RendePassInitInfo initInfo) override;
	void setupPipeline() override;
	void cleanup() override;
	void render(uint32_t frameBufferIndex, uint32_t currentFrame, VkResult acquireImageResult, VkSubmitInfo& info) override;

	void createFrameBuffers();

	void createRenderPass();
	void cleanupSwapChain();
	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t curFrame);
	void recreateSwapChain();

	VkSemaphore m_waitSemaphores[1];
	VkPipelineStageFlags m_waitStages[1];
private:

	
};

INCEPTION_END_NAMESPACE