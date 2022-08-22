#include "icpRenderSystem.h"
#include "icpVulkanRHI.h"
#include <memory>

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

	m_renderPipeline = std::make_shared<icpRenderPipeline>();
	m_renderPipeline->initialize(std::dynamic_pointer_cast<icpVulkanRHI>(m_rhi));

	return true;
}

INCEPTION_END_NAMESPACE