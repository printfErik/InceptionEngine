#pragma once
#include "../../core/icpMacros.h"
#include "icpRenderPassBase.h"

INCEPTION_BEGIN_NAMESPACE

class icpUiPass : public icpRenderPassBase
{
public:
	icpUiPass() = default;
	virtual ~icpUiPass() override;

	void cleanup() override;
	void render(uint32_t frameBufferIndex, uint32_t currentFrame, VkResult acquireImageResult, VkSubmitInfo& info) override;
	void initializeRenderPass(RendePassInitInfo initInfo) override;
	void setupPipeline() override;
	void recreateSwapChain() override;

	void createRenderPass();

	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t curFrameIndex);
	void createFrameBuffers();

	void showDebugUI();
};

INCEPTION_END_NAMESPACE