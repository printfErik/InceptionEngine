#include "icpSystemContainer.h"
#include "icpWindowSystem.h"
#include "icpRenderSystem.h"
#include "icpConfigSystem.h"

INCEPTION_BEGIN_NAMESPACE

icpSystemContainer g_system_container;

static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
	g_system_container.m_renderSystem->setFrameBufferResized(true);
}

void icpSystemContainer::initializeAllSystems(const std::filesystem::path& _configFilePath)
{
	m_configSystem = std::make_shared<icpConfigSystem>(_configFilePath);

	m_windowSystem = std::make_shared<icpWindowSystem>();
	m_windowSystem->initializeWindowSystem();

	m_renderSystem = std::make_shared<icpRenderSystem>();
	m_renderSystem->initializeRenderSystem();

	glfwSetFramebufferSizeCallback(m_windowSystem->getWindow(), framebufferResizeCallback);
}

void icpSystemContainer::shutdownAllSystems()
{
	m_renderSystem.reset();
	m_windowSystem.reset();
	m_configSystem.reset();
}

INCEPTION_END_NAMESPACE