#pragma once
#include "../../core/icpMacros.h"
#include "icpRenderPassBase.h"

INCEPTION_BEGIN_NAMESPACE

class icpEditorUiPass : public icpRenderPassBase
{
public:
	icpEditorUiPass() = default;
	virtual ~icpEditorUiPass() override;

	void InitializeRenderPass(RenderPassInitInfo initInfo) override;
	void SetupPipeline() override;
	void Cleanup() override;
	void Render(uint32_t frameBufferIndex, uint32_t currentFrame, VkResult acquireImageResult, VkSubmitInfo& info) override;

	void CreateFrameBuffers();

	void CreateRenderPass();

	void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t curFrameIndex);
	void RecreateSwapChain() override;

	void AllocateDescriptorSets() override {}
	void AllocateCommandBuffers() override;
	void CreateDescriptorSetLayouts() override {}

private:
	std::shared_ptr<icpEditorUI> m_editorUI;



};

INCEPTION_END_NAMESPACE