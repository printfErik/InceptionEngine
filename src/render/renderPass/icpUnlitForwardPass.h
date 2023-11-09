#pragma once

#include "../../core/icpMacros.h"
#include "icpRenderPassBase.h"

INCEPTION_BEGIN_NAMESPACE

class icpUnlitForwardPass : public icpRenderPassBase
{

public:

	enum eUnlitForwardPassDSType : uint8_t
	{
		PER_MESH = 0,
		PER_MATERIAL,
		PER_FRAME,
		LAYOUT_TYPE_COUNT
	};

	icpUnlitForwardPass() = default;
	~icpUnlitForwardPass() override;

	void InitializeRenderPass(RenderPassInitInfo initInfo) override;
	void SetupPipeline() override;
	void Cleanup() override;
	void Render(uint32_t frameBufferIndex, uint32_t currentFrame, VkResult acquireImageResult, VkSubmitInfo& info) override;

	void RecreateSwapChain() override;
	void CreateFrameBuffers();
	void CreateRenderPass();
	void CreateDescriptorSetLayouts() override;
	void AllocateDescriptorSets() override{};

	void AllocateCommandBuffers() override;

};

INCEPTION_END_NAMESPACE