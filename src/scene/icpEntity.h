#pragma once

#include "../core/icpMacros.h"
#include "../core/icpGuid.h"

#include "icpComponent.h"

#include <entt/entt.hpp>

INCEPTION_BEGIN_NAMESPACE

class icpGameEntity
{
public:
	icpGameEntity();
	virtual ~icpGameEntity() = default;

	void initializeEntity(entt::entity entityHandle, const std::string name);

	void registerComponent(std::weak_ptr<icpComponentBase> comp);
	void uninstallComponent(std::weak_ptr<icpComponentBase> comp);

private:

	icpGuid m_guid;
	std::string m_name;

	entt::entity m_entityHandle;

	std::vector<std::weak_ptr<icpComponentBase>> m_components;

};


INCEPTION_END_NAMESPACE