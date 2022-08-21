#pragma once
#include "icpMacros.h"
#include "icpVulkanRHI.h"
#include "vulkan/vulkan.hpp"

INCEPTION_BEGIN_NAMESPACE

class icpPipelineBase
{
public:
	icpPipelineBase();
	virtual ~icpPipelineBase();

	virtual bool initialize(std::shared_ptr<icpVulkanRHI> rhi) = 0;

};

INCEPTION_END_NAMESPACE