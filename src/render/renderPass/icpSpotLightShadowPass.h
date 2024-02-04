#pragma once
#include "../../core/icpMacros.h"
#include "icpRenderPassBase.h"

INCEPTION_BEGIN_NAMESPACE

class icpSpotLightShadowPass : public icpRenderPassBase
{
public:

	enum eSpotLightShadowPassDSType : uint8_t
	{
		PER_MESH = 0,
		LAYOUT_TYPE_COUNT
	};

	icpSpotLightShadowPass();
	virtual ~icpSpotLightShadowPass() override;

	void InitializeRenderPass(RenderPassInitInfo initInfo) override;
	void SetupPipeline() override;
	void Cleanup() override;
	void Render(uint32_t frameBufferIndex, uint32_t currentFrame, VkResult acquireImageResult) override;
	void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t curFrame);

	void CreateDescriptorSetLayouts() override;
	void AllocateDescriptorSets() override;
	void UpdateRenderPassCB(uint32_t curFrame) override;


private:

	void CreateShadowRenderPass();
	void CreateShadowFrameBuffer();
	void CreateShadowAttachment();

	uint32_t m_nShadowMapDim;
	float m_fDepthBiasConstantFactor;
	float m_fDepthBiasClamp;
	float m_fDepthBiasSlopeFactor;

	VkRenderPass m_shadowRenderPass;
	std::vector<VkFramebuffer> m_vShadowFrameBuffers;

	VkImage m_spotLightShadowArray;
	VkImageView m_spotLightShadowArrayView;
	VmaAllocation m_spotLightShadowArrayAllocation;
};

INCEPTION_END_NAMESPACE