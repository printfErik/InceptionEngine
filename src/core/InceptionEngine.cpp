#include "InceptionEngine.h"
#include "icpWindowSystem.h"
#include "icpRenderSystem.h"
#include "icpConfigSystem.h"
#include "icpSystemContainer.h"


INCEPTION_BEGIN_NAMESPACE
InceptionEngine::~InceptionEngine()
{

}

bool InceptionEngine::initializeEngine(const std::filesystem::path& _configFilePath)
{
	g_system_container.initializeAllSystems(_configFilePath);
	return true;
}

void InceptionEngine::startEngine()
{
	while(!g_system_container.m_windowSystem->shouldClose())
	{
		g_system_container.m_windowSystem->pollEvent();
		g_system_container.m_renderSystem->drawFrame();
	}
}

bool InceptionEngine::stopEngine()
{
	g_system_container.shutdownAllSystems();
	return true;
}

INCEPTION_END_NAMESPACE