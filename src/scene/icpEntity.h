#pragma once

#include "../core/icpMacros.h"

#include "icpSceneSystem.h"

#include <entt/entt.hpp>

INCEPTION_BEGIN_NAMESPACE

class icpGameEntity : public std::enable_shared_from_this<icpGameEntity>
{
public:
	icpGameEntity();
	icpGameEntity(entt::entity enttHandler);
	virtual ~icpGameEntity() = default;

	void InitializeEntity(entt::entity entityHandle, std::shared_ptr<icpGameEntity> parent);

	template<typename T, typename... Args>
	T& installComponent(Args&&... args)
	{
		assert(!hasComponent<T>());
		const auto ptr = m_pSceneSystem.lock();
		T& component = ptr->m_registry.emplace<T>(m_entityHandle, std::forward<Args>(args)...);
		component.m_possessor = this;
		return component;
	}

	template<typename T>
	bool hasComponent()
	{
		const auto ptr = m_pSceneSystem.lock();
		return ptr->m_registry.any_of<T>(m_entityHandle);
	}

	template<typename T>
	T& accessComponent()
	{
		assert(hasComponent<T>());
		const auto ptr = m_pSceneSystem.lock();
		return ptr->m_registry.get<T>(m_entityHandle);
	}

	template<typename T>
	void uninstallComponent()
	{
		assert(hasComponent<T>());
		const auto ptr = m_pSceneSystem.lock();
		ptr->m_registry.remove<T>(m_entityHandle);
	}

	std::shared_ptr<icpGameEntity> GetSharedFromThis()
	{
		return shared_from_this();
	}

private:

	entt::entity m_entityHandle{};

	std::weak_ptr<icpSceneSystem> m_pSceneSystem;
};


INCEPTION_END_NAMESPACE