#pragma once
#include "../core/icpMacros.h"

#include "icpRHI.h"
#include "icpRenderPipeline.h"

INCEPTION_BEGIN_NAMESPACE

class icpRenderSystem
{
public:
	icpRenderSystem();
	~icpRenderSystem();

	bool initializeRenderSystem();
	void drawFrame();
	void setFrameBufferResized(bool _isResized);

private:
	std::shared_ptr<icpRHIBase> m_rhi;
	std::shared_ptr<icpRenderPipeline> m_renderPipeline;
};

INCEPTION_END_NAMESPACE