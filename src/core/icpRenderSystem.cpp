#include "icpRenderSystem.h"
#include "icpVulkanRHI.h"

INCEPTION_BEGIN_NAMESPACE
	icpRenderSystem::icpRenderSystem()
{

}

icpRenderSystem::~icpRenderSystem()
{

}

bool icpRenderSystem::initializeRenderSystem(std::shared_ptr<icpWindowSystem> window_system)
{
	m_rhi = std::make_shared<icpVulkanRHI>();
	m_rhi->initialize(window_system);
	return true;
}

INCEPTION_END_NAMESPACE