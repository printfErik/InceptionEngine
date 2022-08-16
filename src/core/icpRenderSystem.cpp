#include "icpRenderSystem.h"
#include "icpVulkanRHI.h"

INCEPTION_BEGIN_NAMESPACE

icpRenderSystem::icpRenderSystem()
{

}

icpRenderSystem::~icpRenderSystem()
{

}

bool icpRenderSystem::initializeRenderSystem()
{
	m_rhi = std::make_shared<icpVulkanRHI>();
	m_rhi->initialize();
	return true;
}

INCEPTION_END_NAMESPACE