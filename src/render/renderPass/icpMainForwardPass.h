#include "../../core/icpMacros.h"
#include "icpRenderPassBase.h"

INCEPTION_BEGIN_NAMESPACE

class icpMainForwardPass : public icpRenderPassBase
{
public:
	icpMainForwardPass() = default;

	void initializeRenderPass(RendePassInitInfo initInfo) override;

	void setupPipeline() override;

	void createFrameBuffers();

	void createRenderPass();
	void cleanup() override;
	void render() override;

	void cleanupSwapChain();
	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
	void recreateSwapChain();
private:

	uint32_t m_currentFrame = 0;
};

INCEPTION_END_NAMESPACE