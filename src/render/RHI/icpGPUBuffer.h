#pragma once

#include "../../core/icpMacros.h"
#include "icpGPUDevice.h"

INCEPTION_BEGIN_NAMESPACE

struct icpBufferRenderResource
{
	std::shared_ptr<icpRHIBuffer> buffer = nullptr;
	uint64_t range = 0;
	uint64_t offset = 0;
};

INCEPTION_END_NAMESPACE
