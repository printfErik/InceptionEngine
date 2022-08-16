#pragma once
#include "icpMacros.h"
#include "icpRHI.h"

INCEPTION_BEGIN_NAMESPACE

class icpVulkanRHI : public icpRHIBase
{
public:
	icpVulkanRHI() = default;
	~icpVulkanRHI() override;

	bool initialize() override;

private:
	void createInstance();
};

INCEPTION_END_NAMESPACE