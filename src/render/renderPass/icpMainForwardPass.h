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
	void render() override;

	void createFrameBuffers();

	void createRenderPass();
	void cleanupSwapChain();
	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
	void recreateSwapChain();

	uint32_t m_currentFrame = 0;
private:

	
};

INCEPTION_END_NAMESPACE