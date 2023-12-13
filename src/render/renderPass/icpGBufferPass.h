#pragma once
#include "../../core/icpMacros.h"
#include "icpRenderPassBase.h"

INCEPTION_BEGIN_NAMESPACE

class icpGBufferPass : public icpRenderPassBase
{
public:

	enum eGBufferPassDSType : uint8_t
	{
		PER_MESH = 0,
		PER_MATERIAL,
		LAYOUT_TYPE_COUNT
	};

	icpGBufferPass();
	virtual ~icpGBufferPass() override;

	void InitializeRenderPass(RenderPassInitInfo initInfo) override;
	void SetupPipeline() override;
	void Cleanup() override;
	void Render(uint32_t frameBufferIndex, uint32_t currentFrame, VkResult acquireImageResult) override;

	void CreateDescriptorSetLayouts() override;
	void AllocateDescriptorSets() override {}
	void UpdateRenderPassCB(uint32_t curFrame) override;

	void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t curFrame);
};

INCEPTION_END_NAMESPACE