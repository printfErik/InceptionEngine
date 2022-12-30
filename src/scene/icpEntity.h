#pragma once

#include "../core/icpMacros.h"

#include "icpSceneSystem.h"

#include <entt/entt.hpp>

INCEPTION_BEGIN_NAMESPACE

class icpGameEntity
{
public:
	icpGameEntity();
	virtual ~icpGameEntity() = default;

	void initializeEntity(entt::entity entityHandle, icpSceneSystem* sceneSystem);

	template<typename T, typename... Args>
	T& installComponent(Args&&... args)
	{
		assert(!hasComponent<T>());
		T& component = m_sceneSystem->m_registry.emplace<T>(m_entityHandle, std::forward<Args>(args)...);
		//m_sceneSystem->OnComponentAdded<T>(*this, component);
		return component;
	}

	template<typename T>
	bool hasComponent()
	{
		return m_sceneSystem->m_registry.any_of<T>(m_entityHandle);
	}

	template<typename T>
	T& accessComponent()
	{
		assert(hasComponent<T>());
		return m_sceneSystem->m_registry.get<T>(m_entityHandle);
	}

	template<typename T>
	void uninstallComponent()
	{
		assert(hasComponent<T>());
		m_sceneSystem->m_registry.remove<T>(m_entityHandle);
	}

private:

	entt::entity m_entityHandle;

	icpSceneSystem* m_sceneSystem;

};


INCEPTION_END_NAMESPACE