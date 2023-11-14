#pragma once
#include "../../core/icpMacros.h"
#include "icpRenderPassBase.h"

INCEPTION_BEGIN_NAMESPACE


class icpMainForwardPass : public icpRenderPassBase
{
public:

	enum eMainForwardPassDSType : uint8_t
	{
		PER_MESH = 0,
		PER_MATERIAL,
		LAYOUT_TYPE_COUNT
	};

	icpMainForwardPass() = default;
	virtual ~icpMainForwardPass() override;

	void InitializeRenderPass(RenderPassInitInfo initInfo) override;
	void SetupPipeline() override;
	void Cleanup() override;
	void Render(uint32_t frameBufferIndex, uint32_t currentFrame, VkResult acquireImageResult) override;

	void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t curFrame);

	void CreateDescriptorSetLayouts() override;
	void AllocateDescriptorSets() override{}
	void UpdateRenderPassCB(uint32_t curFrame) override;
private:

	//void UpdateGlobalBuffers(uint32_t curFrame);
};

INCEPTION_END_NAMESPACE