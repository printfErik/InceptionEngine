#include "icpEntity.h"

INCEPTION_BEGIN_NAMESPACE

icpGameEntity::icpGameEntity()
{
	
}

void icpGameEntity::initializeEntity(entt::entity entityHandle, icpSceneSystem* sceneSystem)
{
	m_entityHandle = entityHandle;
	m_sceneSystem = sceneSystem;
}

/*
void icpGameEntity::registerComponent(std::weak_ptr<icpComponentBase> comp)
{
	m_components.push_back(comp);
}

void icpGameEntity::uninstallComponent(std::weak_ptr<icpComponentBase> comp)
{
	std::erase(std::remove_if(m_components.begin(), m_components.end(), [&](std::weak_ptr<icpComponentBase> comp_)
	{
		return comp_.lock() == comp.lock();
	}));
}
*/

INCEPTION_END_NAMESPACE