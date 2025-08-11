#pragma once
#include "../../core/icpMacros.h"
#include "icpRenderPassBase.h"
#include "../../render/material/icpMaterial.h"
#include "../../scene/icpEntity.h"
#include "../../mesh/icpPrimitiveRendererComponent.h"

INCEPTION_BEGIN_NAMESPACE
class icpGTAOPass : public icpRenderPassBase
{
public:

	icpGTAOPass() = default;
	virtual ~icpGTAOPass() override;

	void InitializeRenderPass(RenderPassInitInfo initInfo) override;
	void SetupPipeline() override;
	void Cleanup() override;
	void Render(uint32_t frameBufferIndex, uint32_t currentFrame, VkResult acquireImageResult) override;
	void Dispatch(uint32_t frameBufferIndex, uint32_t currentFrame, VkResult acquireImageResult) override;
	void UpdateRenderPassCB(uint32_t curFrame) override;

	void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t curFrame);
	void AllocatedRenderPassDescriptorSets() override {}

};

INCEPTION_END_NAMESPACE