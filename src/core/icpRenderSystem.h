#pragma once
#include "icpMacros.h"
#include "icpRHI.h"
#include "icpRenderPipeline.h"

INCEPTION_BEGIN_NAMESPACE

class icpRenderSystem
{
public:
	icpRenderSystem();
	~icpRenderSystem();

	bool initializeRenderSystem(std::shared_ptr<icpWindowSystem> window_system);
	void drawFrame();

private:
	std::shared_ptr<icpRHIBase> m_rhi;
	std::shared_ptr<icpRenderPipeline> m_renderPipeline;
};

INCEPTION_END_NAMESPACE