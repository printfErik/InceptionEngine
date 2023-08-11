#include "icpEntity.h"

#include "icpXFormComponent.h"
#include "../core/icpSystemContainer.h"
INCEPTION_BEGIN_NAMESPACE

icpGameEntity::icpGameEntity()
{
	m_pSceneSystem = g_system_container.m_sceneSystem;
}

void icpGameEntity::InitializeEntity(entt::entity entityHandle, std::shared_ptr<icpGameEntity> parent)
{
	m_entityHandle = entityHandle;
	const auto pSceneSys = m_pSceneSystem.lock();
	if (!parent)
	{
		pSceneSys->m_sceneRoots.push_back(GetSharedFromThis());
	}

	auto&& parentXForm = parent->accessComponent<icpXFormComponent>();

	auto ptr = GetSharedFromThis();
	auto&& xform = ptr->installComponent<icpXFormComponent>();
	xform.m_parent = std::make_shared<icpXFormComponent>(parentXForm);

	parentXForm.m_children.push_back(std::make_shared<icpXFormComponent>(xform));
}


INCEPTION_END_NAMESPACE