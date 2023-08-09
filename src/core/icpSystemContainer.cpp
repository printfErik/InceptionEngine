#include "icpSystemContainer.h"
#include "../render/icpWindowSystem.h"
#include "../render/icpRenderSystem.h"
#include "../render/icpCameraSystem.h"
#include "../resource/icpResourceSystem.h"
#include "../scene/icpSceneSystem.h"
#include "icpLogSystem.h"

#include "icpConfigSystem.h"
#include "../ui/icpUiSystem.h"

INCEPTION_BEGIN_NAMESPACE

icpSystemContainer g_system_container;

static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
	g_system_container.m_renderSystem->setFrameBufferResized(true);
}

void icpSystemContainer::initializeAllSystems(const std::filesystem::path& _configFilePath)
{
	m_logSystem = std::make_shared<icpLogSystem>();

	m_configSystem = std::make_shared<icpConfigSystem>(_configFilePath);

	m_resourceSystem = std::make_shared<icpResourceSystem>();

	m_windowSystem = std::make_shared<icpWindowSystem>();
	m_windowSystem->initializeWindowSystem();

	m_renderSystem = std::make_shared<icpRenderSystem>();
	m_renderSystem->initializeRenderSystem();

	m_sceneSystem = std::make_shared<icpSceneSystem>();
	m_sceneSystem->initializeScene("");

	m_cameraSystem = std::make_shared<icpCameraSystem>();
	m_cameraSystem->initialize();

	m_uiSystem = std::make_shared<icpUiSystem>();
	m_uiSystem->initializeUiCanvas();

	glfwSetFramebufferSizeCallback(m_windowSystem->getWindow(), framebufferResizeCallback);
}

void icpSystemContainer::shutdownAllSystems()
{
	m_renderSystem.reset();
	m_windowSystem.reset();
	m_cameraSystem.reset();
	m_resourceSystem.reset();
	m_configSystem.reset();
	m_logSystem.reset();
}

INCEPTION_END_NAMESPACE