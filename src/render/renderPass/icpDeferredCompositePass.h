#pragma once
#include "../../core/icpMacros.h"
#include "icpRenderPassBase.h"

INCEPTION_BEGIN_NAMESPACE

class icpDeferredCompositePass : public icpRenderPassBase
{
public:

	enum eDeferredCompositePassDSType : uint8_t
	{
		GBUFFER = 0,
		LAYOUT_TYPE_COUNT
	};

    icpDeferredCompositePass();
    virtual ~icpDeferredCompositePass();

    void InitializeRenderPass(RenderPassInitInfo initInfo) override;
	void SetupPipeline() override;
	void Cleanup() override;
	void Render(uint32_t frameBufferIndex, uint32_t currentFrame, VkResult acquireImageResult) override;

	void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t curFrame);

	void CreateDescriptorSetLayouts() override;
	void AllocateDescriptorSets() override;
	void UpdateRenderPassCB(uint32_t curFrame) override;

private:

	std::vector<VkDescriptorSet> m_vGBufferDSs;
};

INCEPTION_END_NAMESPACE