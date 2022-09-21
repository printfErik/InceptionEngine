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
};

INCEPTION_END_NAMESPACE