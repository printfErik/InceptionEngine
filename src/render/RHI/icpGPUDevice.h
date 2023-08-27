#pragma once
#include "../../core/icpMacros.h"
#include "../icpWindowSystem.h"

INCEPTION_BEGIN_NAMESPACE

struct icpDescriptorSetCreation;

class icpGPUDevice
{
public:
	icpGPUDevice() = default;

	virtual	~icpGPUDevice() = 0;

	virtual bool Initialize(std::shared_ptr<icpWindowSystem> window_system) = 0;

	virtual VkDevice GetLogicalDevice() = 0;

	virtual void CreateDescriptorSet(const icpDescriptorSetCreation& creation, std::vector<VkDescriptorSet>& DSs) = 0;
};

inline icpGPUDevice::~icpGPUDevice() = default;

INCEPTION_END_NAMESPACE