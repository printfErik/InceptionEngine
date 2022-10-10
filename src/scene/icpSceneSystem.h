#pragma once
#include "../core/icpMacros.h"
#include "../core/icpGuid.h"

#include <entt/entt.hpp>


INCEPTION_BEGIN_NAMESPACE

class icpSceneSystem
{
public:
	icpSceneSystem() = default;
	~icpSceneSystem() = default;

	void initializeScene(const std::filesystem::path& mapPath);

	void createEntity(const std::string& name);

private:
	entt::registry m_registry;
};


INCEPTION_END_NAMESPACE
