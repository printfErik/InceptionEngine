#pragma once
#include "../../core/icpMacros.h"
#include "../RHI/Vulkan/icpVkGPUDevice.h"
#include "../RHI/icpDescirptorSet.h"

INCEPTION_BEGIN_NAMESPACE

class icpSceneRenderer;
enum class eRenderPass;

class icpEditorUI;

class icpRenderPassBase
{
public:

	struct RenderPassInitInfo
	{
		std::shared_ptr<icpGPUDevice> device{ nullptr };
		eRenderPass passType;
		std::weak_ptr<icpSceneRenderer> sceneRenderer;
		std::shared_ptr<icpEditorUI> editorUi{ nullptr };
	};

	struct RenderPipelineInfo
	{
		VkPipeline m_pipeline;
		VkPipelineLayout m_pipelineLayout;
	};


	icpRenderPassBase() = default;
	virtual ~icpRenderPassBase() {}

	virtual void InitializeRenderPass(RenderPassInitInfo initInfo) = 0;
	virtual void SetupPipeline() = 0;
	virtual void Cleanup() = 0;
	virtual void Render(uint32_t frameBufferIndex, uint32_t currentFrame, VkResult acquireImageResult) = 0;
	virtual void CreateDescriptorSetLayouts() = 0;
	virtual void AllocateDescriptorSets() = 0;
	virtual void UpdateRenderPassCB(uint32_t curFrame) = 0;

	RenderPipelineInfo m_pipelineInfo{};
	std::vector<icpDescriptorSetLayoutInfo> m_DSLayouts;

protected:
	std::weak_ptr<icpSceneRenderer> m_pSceneRenderer;
	std::shared_ptr<icpGPUDevice> m_rhi = nullptr;
};

INCEPTION_END_NAMESPACE