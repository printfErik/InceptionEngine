#include "icpEntity.h"

INCEPTION_BEGIN_NAMESPACE

icpGameEntity::icpGameEntity()
{
	
}

void icpGameEntity::initializeEntity(entt::entity entityHandle, icpSceneSystem* sceneSystem, bool isRoot)
{
	m_entityHandle = entityHandle;
	m_sceneSystem = sceneSystem;

	if (isRoot) 
		m_sceneSystem->m_sceneRoots.push_back(std::make_shared<icpGameEntity>(*this));
}


INCEPTION_END_NAMESPACE