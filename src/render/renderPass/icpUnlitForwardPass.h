#pragma once

#include "../../core/icpMacros.h"
#include "icpRenderPassBase.h"

INCEPTION_BEGIN_NAMESPACE

class icpUnlitForwardPass : public icpRenderPassBase
{
	icpUnlitForwardPass() = default;
	~icpUnlitForwardPass() override;

	void initializeRenderPass(RenderPassInitInfo initInfo) override;
	void setupPipeline() override;
	void cleanup() override;
	void render(uint32_t frameBufferIndex, uint32_t currentFrame, VkResult acquireImageResult, VkSubmitInfo& info) override;

};

INCEPTION_END_NAMESPACE