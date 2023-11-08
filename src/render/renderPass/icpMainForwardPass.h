#pragma once
#include "../../core/icpMacros.h"
#include "icpRenderPassBase.h"

INCEPTION_BEGIN_NAMESPACE


class icpMainForwardPass : public icpRenderPassBase
{
public:

	enum eMainForwardPassDSType : uint8_t
	{
		PER_MESH = 0,
		PER_MATERIAL,
		PER_FRAME,
		LAYOUT_TYPE_COUNT
	};

	icpMainForwardPass() = default;
	virtual ~icpMainForwardPass() override;

	void initializeRenderPass(RenderPassInitInfo initInfo) override;
	void setupPipeline() override;
	void cleanup() override;
	void render(uint32_t frameBufferIndex, uint32_t currentFrame, VkResult acquireImageResult, VkSubmitInfo& info) override;

	void createFrameBuffers();

	void createRenderPass();
	void cleanupSwapChain();
	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t curFrame);
	void recreateSwapChain() override;

	void CreateDescriptorSetLayouts() override;
	void AllocateDescriptorSets() override;

	void AllocateCommandBuffers() override;

	VkSemaphore m_waitSemaphores[1];
	VkPipelineStageFlags m_waitStages[1];

	std::vector<VkDescriptorSet> m_perFrameDSs;

private:

	void UpdateGlobalBuffers(uint32_t curFrame);
};

INCEPTION_END_NAMESPACE