#include "InceptionEngine.h"
#include "icpWindowSystem.h"
#include "icpRenderSystem.h"
#include "icpConfigSystem.h"

#include <filesystem>

INCEPTION_BEGIN_NAMESPACE
InceptionEngine::~InceptionEngine()
{

}

bool InceptionEngine::initializeEngine(const std::filesystem::path& _configFilePath)
{

	m_configSystem = std::make_shared<icpConfigSystem>(_configFilePath);

	m_windowSystem = std::make_shared<icpWindowSystem>();
	m_windowSystem->initializeWindowSystem();

	m_renderSystem = std::make_shared<icpRenderSystem>(_configFilePath);
	m_renderSystem->initializeRenderSystem(m_windowSystem);
	return true;
}

void InceptionEngine::startEngine()
{
	while(!m_windowSystem->shouldClose())
	{
		m_windowSystem->pollEvent();
		m_renderSystem->drawFrame();
	}
}

bool InceptionEngine::stopEngine()
{
	m_renderSystem.reset();
	m_windowSystem.reset();
	return true;
}

INCEPTION_END_NAMESPACE