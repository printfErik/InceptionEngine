//#pragma once
//#include "../../core/icpMacros.h"
//#include "icpRenderPassBase.h"
//
//INCEPTION_BEGIN_NAMESPACE
//
//class icpRenderToImgPass : public icpRenderPassBase
//{
//public:
//	icpRenderToImgPass() = default;
//	virtual ~icpRenderToImgPass() override;
//
//	void cleanup() override;
//	void render(uint32_t frameBufferIndex, uint32_t currentFrame, VkResult acquireImageResult, VkSubmitInfo& info) override;
//	void initializeRenderPass(RendePassInitInfo initInfo) override;
//	void setupPipeline() override;
//	void recreateSwapChain() override;
//
//	void createRenderPass();
//
//	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t curFrameIndex);
//	void createFrameBuffers();
//
//	void showDebugUI();
//
//	void createViewPortImage();
//
//	std::vector<VkFramebuffer> m_viewPortFramebuffers;
//
//private:
//
//};
//
//INCEPTION_END_NAMESPACE