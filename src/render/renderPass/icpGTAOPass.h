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
	void Dispatch(uint32_t frameBufferIndex, uint32_t currentFrame, VkResult acquireImageResult) override;

	void AllocatedRenderPassDescriptorSets() override;

	void SetupPassOutput() override;

private:
	std::vector<VkDescriptorSet> GTAOPassDSs;

	icpTextureRenderResourceInfo AORT;
};

INCEPTION_END_NAMESPACE