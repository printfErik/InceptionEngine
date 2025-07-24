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
	void Render(uint32_t frameBufferIndex, uint32_t currentFrame, VkResult acquireImageResult) override;
	void UpdateRenderPassCB(uint32_t curFrame) override {}
private:
	std::shared_ptr<icpEditorUI> m_editorUI;



};

INCEPTION_END_NAMESPACE