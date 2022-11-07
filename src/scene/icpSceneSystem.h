#pragma once
#include "../core/icpMacros.h"
#include "../core/icpGuid.h"

#include <entt/entt.hpp>


INCEPTION_BEGIN_NAMESPACE

class icpGameEntity;

class icpSceneSystem
{
public:
	icpSceneSystem() = default;
	~icpSceneSystem() = default;

	void initializeScene(const std::filesystem::path& mapPath);

	icpGameEntity createEntity(const std::string& name);

	void saveScene(const std::filesystem::path& outPath);


	entt::registry m_registry;

private:

	void loadSceneFromMapPath(const std::filesystem::path& mapPath);
};


INCEPTION_END_NAMESPACE
