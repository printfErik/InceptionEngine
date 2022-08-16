#pragma once
#include "icpMacros.h"
#include "icpRHI.h"
INCEPTION_BEGIN_NAMESPACE

class icpRenderSystem
{
public:
	icpRenderSystem();
	~icpRenderSystem();

	bool initializeRenderSystem();

private:
	std::shared_ptr<icpRHIBase> m_rhi;
};



INCEPTION_END_NAMESPACE