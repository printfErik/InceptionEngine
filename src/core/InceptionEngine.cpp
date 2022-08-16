#include "InceptionEngine.h"
#include "icpRenderSystem.h"


INCEPTION_BEGIN_NAMESPACE
InceptionEngine::~InceptionEngine()
{

}

bool InceptionEngine::initializeEngine()
{
	m_renderSystem = std::make_shared<icpRenderSystem>();
	m_renderSystem->initializeRenderSystem();
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