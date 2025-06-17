#pragma once
#include "../../core/icpMacros.h"

#include "../RHI/icpGPUDevice.h"


INCEPTION_BEGIN_NAMESPACE

// todo
static uint32_t s_csmCascadeCount(4u);
static uint32_t s_cascadeShadowMapResolution(1024u);

class icpShadowManager
{
public:
	icpShadowManager() = default;
	virtual ~icpShadowManager() = default;

	void UpdateCSMCB();
	
};


INCEPTION_END_NAMESPACE