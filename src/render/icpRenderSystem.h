#pragma once
#include "../core/icpMacros.h"

#include "icpRHI.h"
#include "icpRenderPassManager.h"

INCEPTION_BEGIN_NAMESPACE

class icpRenderSystem
{
public:
	icpRenderSystem();
	~icpRenderSystem();

	bool initializeRenderSystem();
	void drawFrame();
	void setFrameBufferResized(bool _isResized);


	std::shared_ptr<icpRHIBase> m_rhi;
	std::shared_ptr<icpRenderPassManager> m_renderPassManager;

private:
};

INCEPTION_END_NAMESPACE