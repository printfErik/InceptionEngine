#pragma once
#include "../core/icpMacros.h"
#include <vulkan/vulkan.hpp>

#include "RHI/icpDescirptorSet.h"
#include "RHI/Vulkan/icpVkGPUDevice.h"
#include "icpSceneRenderer.h"

INCEPTION_BEGIN_NAMESPACE

class icpRenderPassBase;

class icpForwardSceneRenderer : public std::enable_shared_from_this<icpForwardSceneRenderer>, public icpSceneRenderer
{
public:
	icpForwardSceneRenderer();
	virtual ~icpForwardSceneRenderer() override;

	bool Initialize(std::shared_ptr<icpGPUDevice> vulkanRHI) override;
	void Cleanup() override;
	void Render() override;

	VkCommandBuffer GetMainForwardCommandBuffer(uint32_t curFrame) override;
	VkRenderPass GetMainForwardRenderPass() override;
	VkDescriptorSet GetSceneDescriptorSet(uint32_t curFrame) override;

	icpDescriptorSetLayoutInfo& GetSceneDSLayout() override;

	VkRenderPass GetGBufferRenderPass() override { return VK_NULL_HANDLE; }

	void CreateForwardRenderPass();
	void CreateSwapChainFrameBuffers();
	void AllocateCommandBuffers();

	void RecreateSwapChain();
	void CleanupSwapChain();

private:

	void ResetThenBeginCommandBuffer();
	void BeginForwardRenderPass(uint32_t imageIndex);

	void EndForwardRenderPass();
	void EndRecordingCommandBuffer();

	void SubmitCommandList();
	void Present(uint32_t imageIndex);

	std::vector<VkCommandBuffer> m_vMainForwardCommandBuffers;
	VkRenderPass m_mainForwardRenderPass{ VK_NULL_HANDLE };
	
	std::vector<VkFramebuffer> m_vSwapChainFrameBuffers;
};

INCEPTION_END_NAMESPACE