#pragma once
#include "../../core/icpMacros.h"
#include "icpRenderPassBase.h"


INCEPTION_BEGIN_NAMESPACE
class icpForwardTranslucentPass : public icpRenderPassBase
{
public:

	icpForwardTranslucentPass() = default;
	virtual ~icpForwardTranslucentPass() override;

	void InitializeRenderPass(RenderPassInitInfo initInfo) override;
	void SetupPipeline() override;
	void Cleanup() override;
	void Render(uint32_t frameBufferIndex, uint32_t currentFrame, VkResult acquireImageResult) override;

	void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t curFrame);
	void UpdateRenderPassCB(uint32_t curFrame) override;

	void AllocatedRenderPassDescriptorSets() override {}
	void BeginRenderingCreateInfo(VkCommandBuffer cmdBuf, uint32_t imageIndex) override;

	void SetupPassOutput() override;
private:

	//void UpdateGlobalBuffers(uint32_t curFrame);
};



INCEPTION_END_NAMESPACE