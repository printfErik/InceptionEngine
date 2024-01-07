#pragma once
#include "../../core/icpMacros.h"
#include "icpRenderPassBase.h"

INCEPTION_BEGIN_NAMESPACE

class icpShadowPass : public icpRenderPassBase
{
public:
	icpShadowPass();
	virtual ~icpShadowPass() override;

	void InitializeRenderPass(RenderPassInitInfo initInfo) override;
	void SetupPipeline() override;
	void Cleanup() override;
	void Render(uint32_t frameBufferIndex, uint32_t currentFrame, VkResult acquireImageResult) override {}

	void CreateDescriptorSetLayouts() override {}
	void AllocateDescriptorSets() override {}
	void UpdateRenderPassCB(uint32_t curFrame) override{}


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

	VkImage m_shadowCubeMapArray;
	VkImageView m_shadowCubeMapArrayView;
	VmaAllocation m_shadowCubeMapArrayAllocation;
};

INCEPTION_END_NAMESPACE