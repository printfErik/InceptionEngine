#include "icpSceneSystem.h"
#include "icpEntity.h"

INCEPTION_BEGIN_NAMESPACE

void icpSceneSystem::initializeScene(const std::filesystem::path& mapPath)
{
	
}

void icpSceneSystem::createEntity(const std::string& name)
{
	icpGameEntity entity;
	entity.initializeEntity(m_registry.create(), name);
}




INCEPTION_END_NAMESPACE