#pragma once
#include "../core/icpMacros.h"
#include <vulkan/vulkan.hpp>
#include "icpVulkanRHI.h"

INCEPTION_BEGIN_NAMESPACE

class icpRenderPassBase;

class icpRenderPassManager
{
public:
	icpRenderPassManager();
	~icpRenderPassManager();

	bool initialize(std::shared_ptr<icpVulkanRHI> vulkanRHI);
	void cleanup();

	void render();

private:

	std::shared_ptr<icpVulkanRHI> m_rhi = nullptr;

	std::vector<std::shared_ptr<icpRenderPassBase>> m_renderPasses;
};

INCEPTION_END_NAMESPACE