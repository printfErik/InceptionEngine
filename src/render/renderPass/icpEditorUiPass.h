#pragma once
#include "../../core/icpMacros.h"
#include "icpRenderPassBase.h"

INCEPTION_BEGIN_NAMESPACE

class icpEditorUiPass : public icpRenderPassBase
{
public:
	icpEditorUiPass() = default;
	virtual ~icpEditorUiPass() override;

	void initializeRenderPass(RenderPassInitInfo initInfo) override;
	void setupPipeline() override;
	void cleanup() override;
	void render(uint32_t frameBufferIndex, uint32_t currentFrame, VkResult acquireImageResult, VkSubmitInfo& info) override;

	void createFrameBuffers();

	void createRenderPass();

	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t curFrameIndex);
	void recreateSwapChain() override;

	void allocateDescriptorSets() override {}
	void createDescriptorSetLayouts() override {}

private:
	std::shared_ptr<icpEditorUI> m_editorUI;



};

INCEPTION_END_NAMESPACE