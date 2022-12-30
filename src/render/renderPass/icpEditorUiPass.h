#pragma once
#include "../../core/icpMacros.h"
#include "icpRenderPassBase.h"

INCEPTION_BEGIN_NAMESPACE

class icpEditorUiPass : public icpRenderPassBase
{
public:
	icpEditorUiPass() = default;
	virtual ~icpEditorUiPass() override;

	void initializeRenderPass(RendePassInitInfo initInfo) override;
	void setupPipeline() override;
	void cleanup() override;
	void render(uint32_t frameBufferIndex, uint32_t currentFrame, VkResult acquireImageResult, VkSubmitInfo& info) override;

	void createFrameBuffers();

	void createRenderPass();

	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t curFrameIndex);
	void recreateSwapChain() override;

	//void createViewPortImage();

	//std::vector<VkFramebuffer> m_viewPortFramebuffers;

	//std::vector<VkDescriptorSet> m_Dset;

private:
	std::shared_ptr<icpEditorUI> m_editorUI;

	//std::vector<VkImage> m_viewPortImages;
	//std::vector<VkDeviceMemory> m_viewPortImageDeviceMemory;
	//std::vector<VkImageView> m_viewPortImageViews;

};

INCEPTION_END_NAMESPACE