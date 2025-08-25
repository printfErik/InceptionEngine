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

	VkCommandBuffer GetGBufferCommandBuffer(uint32_t curFrame);
	VkCommandBuffer GetGTAOCommandBuffer(uint32_t curFrame);
	VkCommandBuffer GetLightingCommandBuffer(uint32_t curFrame);

	void RecreateSwapChain();
	void CleanupSwapChain();

	void AllocateCommandBuffers();

	void CreateSemaphores();

	void ImageBarrier(
		VkCommandBuffer cmdBuf,
		VkAccessFlags srcAccess, VkAccessFlags dstAccess,
		VkImageLayout oldLayout, VkImageLayout newLayout,
		uint32_t srcQueueFamilyIndex, uint32_t dstQueueFamilyIndex,
		VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
		VkImage image, const VkImageSubresourceRange& subresourceRange
	);
	
private:
	void BeginCommandBuffer(VkCommandBuffer cb);
	void EndRecordingCommandBuffer(VkCommandBuffer cb);

	void SubmitCommandList(
		VkCommandBuffer cmdBuffer,
		VkSemaphore waitSemaphore,
		VkPipelineStageFlags waitStage,
		VkSemaphore signalSemaphore);
	void Present(uint32_t imageIndex);

	std::vector<VkCommandBuffer> m_GBufferCommandBuffers;
	std::vector<VkCommandBuffer> m_AOCommandBuffers;
	std::vector<VkCommandBuffer> m_LightingCommandBuffers;

	std::vector<VkSemaphore> m_GBufferFinishSemaphores;
	std::vector<VkSemaphore> m_GTAOFinishSemaphores;
	std::vector<VkSemaphore> m_imageAvailableForRenderingSemaphores;
	std::vector<VkSemaphore> m_renderFinishedForPresentationSemaphores;
};


INCEPTION_END_NAMESPACE