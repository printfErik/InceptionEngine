#pragma once

#include "../core/icpMacros.h"
#include "icpSceneRenderer.h"

INCEPTION_BEGIN_NAMESPACE

class icpDeferredRenderer : public std::enable_shared_from_this<icpDeferredRenderer>, public icpSceneRenderer
{
public:

	enum class eDeferredLightingType
	{
		SIMPLE = 0,
		TILE,
		CLUSTER
	};

	icpDeferredRenderer() = default;
	virtual ~icpDeferredRenderer() override;

	bool Initialize(std::shared_ptr<icpGPUDevice> vulkanRHI) override;
	void Cleanup() override;
	void Render() override;
	void AllocateGlobalSceneDescriptorSets() override;

	VkCommandBuffer GetDeferredCommandBuffer(uint32_t curFrame) override;

	void RecreateSwapChain();
	void CleanupSwapChain();

	void AllocateCommandBuffers();
	
private:
	void ResetThenBeginCommandBuffer();

	void EndDeferredRenderPass();
	void EndRecordingCommandBuffer();

	void SubmitCommandList();
	void Present(uint32_t imageIndex);

	std::vector<VkCommandBuffer> m_vDeferredCommandBuffers;
};


INCEPTION_END_NAMESPACE