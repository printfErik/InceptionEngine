#pragma once
#include "../core/icpMacros.h"
#include "../core/icpGuid.h"

#include "../fb_gen/Scene_generated.h"
#include "../fb_gen/component_generated.h"

#include <entt/entt.hpp>

INCEPTION_BEGIN_NAMESPACE

class icpResourceBase;
class icpGameEntity;

class icpSceneSystem
{
public:
	icpSceneSystem() = default;
	~icpSceneSystem() = default;

	void initializeScene(const std::filesystem::path& mapPath);

	std::shared_ptr<icpGameEntity> CreateEntity(const std::string& name, std::shared_ptr<icpGameEntity> parent);

	void saveScene(const std::filesystem::path& outPath);

	void getRootEntityList(std::vector<std::shared_ptr<icpGameEntity>>& list);
	void createMeshEntityFromResource(std::shared_ptr<icpResourceBase> meshRes);

	void LoadDefaultScene();

	entt::registry m_registry;

	std::vector<std::shared_ptr<icpGameEntity>> m_sceneRoots;

private:

	void loadSceneFromMapPath(const std::filesystem::path& mapPath);
	void addRootNodeToHierarchy(const fb::flatbufferTreeNode* node);
	void recursiveAddNodeToHierarchy(const fb::flatbufferTreeNode* node, std::shared_ptr<icpGameEntity> parent);

	icpGameEntity createEntityFromMap(const fb::flatbufferTreeNode* node);


};


INCEPTION_END_NAMESPACE
