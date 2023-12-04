#pragma once
#include "../core/icpMacros.h"
#include "RHI/icpDescirptorSet.h"
#include "RHI/icpGPUDevice.h"


INCEPTION_BEGIN_NAMESPACE

class icpRenderPassBase;


enum class eRenderPass
{
	MAIN_FORWARD_PASS = 0,
	UNLIT_PASS,
	EDITOR_UI_PASS,
	RENDER_PASS_COUNT
};

class icpSceneRenderer
{
public:
	icpSceneRenderer() = default;
	virtual ~icpSceneRenderer() = 0;

	virtual bool Initialize(std::shared_ptr<icpGPUDevice> vulkanRHI) = 0;
	virtual void Cleanup();
	virtual void Render() = 0;

	virtual VkCommandBuffer GetMainForwardCommandBuffer(uint32_t curFrame) = 0;
	virtual VkRenderPass GetMainForwardRenderPass() = 0;
	virtual icpDescriptorSetLayoutInfo& GetSceneDSLayout() = 0;
	virtual VkDescriptorSet GetSceneDescriptorSet(uint32_t curFrame) = 0;

	std::shared_ptr<icpRenderPassBase> AccessRenderPass(eRenderPass passType);

protected:

	std::shared_ptr<icpGPUDevice> m_pDevice = nullptr;
	std::vector<std::shared_ptr<icpRenderPassBase>> m_renderPasses;
private:
};

inline icpSceneRenderer::~icpSceneRenderer() = default;

INCEPTION_END_NAMESPACE