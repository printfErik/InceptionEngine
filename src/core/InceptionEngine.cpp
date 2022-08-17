#include "InceptionEngine.h"
#include "icpWindowSystem.h"
#include "icpRenderSystem.h"


INCEPTION_BEGIN_NAMESPACE
InceptionEngine::~InceptionEngine()
{

}

bool InceptionEngine::initializeEngine()
{
	m_windowSystem = std::make_shared<icpWindowSystem>();
	m_windowSystem->initializeWindowSystem();

	m_renderSystem = std::make_shared<icpRenderSystem>();
	m_renderSystem->initializeRenderSystem(m_windowSystem);
	return true;
}

void InceptionEngine::startEngine()
{

}

bool InceptionEngine::stopEngine()
{
	return true;
}

INCEPTION_END_NAMESPACE