#pragma once
#include "../../core/icpMacros.h"
#include "icpRenderPassBase.h"

INCEPTION_BEGIN_NAMESPACE

class icpUiPass : public icpRenderPassBase
{
public:
	icpUiPass() = default;
	virtual ~icpUiPass() override;

	void cleanup() override;
	void render() override;
	void initializeRenderPass(RendePassInitInfo initInfo) override;
	void setupPipeline() override;

};

INCEPTION_END_NAMESPACE