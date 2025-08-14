#pragma once
#include "../../core/icpMacros.h"
#include "../RHI/Vulkan/icpVkGPUDevice.h"
#include "../RHI/icpDescriptorSet.h"

INCEPTION_BEGIN_NAMESPACE

class icpSceneRenderer;
enum class eRenderPass;

class icpEditorUI;

class icpRenderPassBase
{
public:

	struct RenderPassInitInfo
	{
		std::shared_ptr<icpGPUDevice> device = nullptr;
		std::weak_ptr<icpSceneRenderer> sceneRenderer;
		std::vector<icpRenderPassBase> renderPassDependencies;
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
	virtual void AllocatedRenderPassDescriptorSets() = 0;
	virtual void SetupPassOutput() = 0;

	virtual void Cleanup() = 0;

	virtual void Render(uint32_t frameBufferIndex, uint32_t currentFrame, VkResult acquireImageResult) {}
	virtual void Dispatch(uint32_t frameBufferIndex, uint32_t currentFrame, VkResult acquireImageResult) {}
	virtual void BeginRenderingCreateInfo( VkCommandBuffer cmdBuf, uint32_t imageIndex) {}
	virtual void UpdateRenderPassCB(uint32_t curFrame) {}

	void AddPassInputLayout(VkDescriptorSetLayout layout);

	RenderPipelineInfo m_pipelineInfo{};
	std::vector<VkDescriptorSetLayout> dsLayouts;

protected:
	std::weak_ptr<icpSceneRenderer> m_pSceneRenderer;
	std::shared_ptr<icpGPUDevice> m_pDevice = nullptr;
};

INCEPTION_END_NAMESPACE