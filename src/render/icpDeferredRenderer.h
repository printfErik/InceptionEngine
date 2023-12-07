#pragma once

#include "../core/icpMacros.h"
#include "icpSceneRenderer.h"

INCEPTION_BEGIN_NAMESPACE

class icpDeferredRenderer : public std::enable_shared_from_this<icpDeferredRenderer>, public icpSceneRenderer
{
public:
	icpDeferredRenderer() = default;
	virtual ~icpDeferredRenderer() override;

	bool Initialize(std::shared_ptr<icpGPUDevice> vulkanRHI) override;
	void Cleanup() override;
	void Render() override;

	VkCommandBuffer GetMainForwardCommandBuffer(uint32_t curFrame) override{ return VK_NULL_HANDLE; }
	VkRenderPass GetMainForwardRenderPass() override{ return VK_NULL_HANDLE; }

	VkRenderPass GetGBufferRenderPass() override;
	VkCommandBuffer GetDeferredCommandBuffer(uint32_t curFrame) override;

	void RecreateSwapChain();
	void CleanupSwapChain();

	void AllocateCommandBuffers();

	VkImageView GetGBufferAView();
private:

	void CreateGBufferAttachments();
	void CreateDeferredFrameBuffer();
	void CreateDeferredRenderPass();

	void ResetThenBeginCommandBuffer();
	void BeginForwardRenderPass(uint32_t imageIndex);

	void EndForwardRenderPass();
	void EndRecordingCommandBuffer();

	VkImage m_gBufferA{ VK_NULL_HANDLE };
	VkImageView m_gBufferAView{ VK_NULL_HANDLE };
	VmaAllocation m_gBufferAAllocation{ VK_NULL_HANDLE };

	VkImage m_gBufferB{ VK_NULL_HANDLE };
	VkImageView m_gBufferBView{ VK_NULL_HANDLE };
	VmaAllocation m_gBufferBAllocation{ VK_NULL_HANDLE };

	VkImage m_gBufferC{ VK_NULL_HANDLE };
	VkImageView m_gBufferCView{ VK_NULL_HANDLE };
	VmaAllocation m_gBufferCAllocation{ VK_NULL_HANDLE };

	VkRenderPass m_deferredRenderPass{ VK_NULL_HANDLE };

	std::vector<VkFramebuffer> m_vDeferredFrameBuffers;

	icpDescriptorSetLayoutInfo m_sceneDeferredDSLayout{};

	std::vector<VkCommandBuffer> m_vDeferredCommandBuffers;
};


INCEPTION_END_NAMESPACE