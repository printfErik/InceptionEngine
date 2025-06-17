#pragma once
#include "../../core/icpMacros.h"
#include "icpRenderPassBase.h"

INCEPTION_BEGIN_NAMESPACE

class icpCSMPass : public icpRenderPassBase
{
public:

	enum eCSMPassDSType : uint8_t
	{
		PER_MESH = 0,
		LAYOUT_TYPE_COUNT
	};

	icpCSMPass();
	virtual ~icpCSMPass() override;

	void InitializeRenderPass(RenderPassInitInfo initInfo) override;
	void SetupPipeline() override;
	void Cleanup() override;
	void Render(uint32_t frameBufferIndex, uint32_t currentFrame, VkResult acquireImageResult) override;
	void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t curFrame);

	void CreateDescriptorSetLayouts() override;
	void AllocateDescriptorSets() override;
	void UpdateRenderPassCB(uint32_t curFrame) override;


private:

	void CreateCSMRenderPass();
	void CreateCSMFrameBuffer();
	void CreateCSMImageRenderResource();

	float m_fDepthBiasConstantFactor;
	float m_fDepthBiasClamp;
	float m_fDepthBiasSlopeFactor;

	VkRenderPass m_shadowRenderPass;
	std::vector<VkFramebuffer> m_csmFrameBuffers;

	VkImage m_csmArray;
	VkImageView m_csmArrayView;
	VmaAllocation m_csmArrayAllocation;
};


INCEPTION_END_NAMESPACE