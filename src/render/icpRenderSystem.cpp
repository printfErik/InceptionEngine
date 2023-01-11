#include "icpRenderSystem.h"
#include "icpVulkanRHI.h"
#include "../core/icpSystemContainer.h"
#include "../scene/icpSceneSystem.h"
#include "../scene/icpXFormComponent.h"

INCEPTION_BEGIN_NAMESPACE

icpRenderSystem::icpRenderSystem()
{
}

icpRenderSystem::~icpRenderSystem()
{
	m_renderPassManager.reset();
	m_rhi.reset();
}

bool icpRenderSystem::initializeRenderSystem()
{
	m_rhi = std::make_shared<icpVulkanRHI>();
	m_rhi->initialize(g_system_container.m_windowSystem);

	m_renderPassManager = std::make_shared<icpRenderPassManager>();
	m_renderPassManager->initialize(std::dynamic_pointer_cast<icpVulkanRHI>(m_rhi));

	return true;
}

void icpRenderSystem::drawFrame()
{
	m_renderPassManager->render();
}

void icpRenderSystem::setFrameBufferResized(bool _isResized)
{
	std::dynamic_pointer_cast<icpVulkanRHI>(m_rhi)->m_framebufferResized = _isResized;
}

void icpRenderSystem::getAllStaticMeshRenderers()
{
	
}

void icpRenderSystem::drawCube()
{
	auto cubeEntity = g_system_container.m_sceneSystem->createEntity("Cube", true);

	auto&& meshRenderer = cubeEntity.installComponent<icpMeshRendererComponent>();

	meshRenderer.m_primitive = ePrimitiveType::CUBE;
}


INCEPTION_END_NAMESPACE