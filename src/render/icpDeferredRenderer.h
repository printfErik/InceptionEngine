#pragma once

#include "../../core/icpMacros.h"
#include "icpSceneRenderer.h"

INCEPTION_BEGIN_NAMESPACE

class icpDeferredRenderer : public std::enable_shared_from_this<icpDeferredRenderer>, public icpSceneRenderer
{
public:
	icpDeferredRenderer() = default;
	virtual ~icpDeferredRenderer() = 0;

	bool Initialize(std::shared_ptr<icpGPUDevice> vulkanRHI) override{}
	void Cleanup() override{}
	void Render() override{}

	VkCommandBuffer GetMainForwardCommandBuffer(uint32_t curFrame) override{}
	VkRenderPass GetMainForwardRenderPass() override{}
	VkDescriptorSet GetSceneDescriptorSet(uint32_t curFrame) override{}

	VkRenderPass GetGBufferRenderPass() override;
	icpDescriptorSetLayoutInfo& GetSceneDSLayout() override{}

private:


};


INCEPTION_END_NAMESPACE