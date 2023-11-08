#pragma once
#include "../core/icpMacros.h"
#include "../core/icpGuid.h"

#include "../fb_gen/Scene_generated.h"
#include "../fb_gen/component_generated.h"

#include <entt/entt.hpp>

#include "glm/fwd.hpp"
#include "glm/vec3.hpp"

INCEPTION_BEGIN_NAMESPACE
class icpMeshResource;
class icpResourceBase;
class icpGameEntity;


#define MAX_POINT_LIGHT_COUNT 4

struct DirectionalLightRenderResource
{
	glm::vec4 direction;
	glm::vec4 color;
};

struct PointLightRenderResource
{
	glm::vec3 position;
	glm::vec3 ambient;
	glm::vec3 diffuse;
	glm::vec3 specular;
	float constant = 0.f;
	float linear = 0.f;
	float quadratic = 0.f;
};

struct perFrameCB
{
	glm::mat4 view;
	glm::mat4 projection;
	glm::vec3 camPos;
	float pointLightNumber = 0.f;
	DirectionalLightRenderResource dirLight;
	PointLightRenderResource pointLight[MAX_POINT_LIGHT_COUNT];
};

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

	std::shared_ptr<icpGameEntity> FindEntity(icpGuid guid);

	void CreateSceneCB();

	entt::registry m_registry;

	std::vector<std::shared_ptr<icpGameEntity>> m_sceneRoots;

private:

	void loadSceneFromMapPath(const std::filesystem::path& mapPath);
	void addRootNodeToHierarchy(const fb::flatbufferTreeNode* node);
	void recursiveAddNodeToHierarchy(const fb::flatbufferTreeNode* node, std::shared_ptr<icpGameEntity> parent);

	icpGameEntity createEntityFromMap(const fb::flatbufferTreeNode* node);


};


INCEPTION_END_NAMESPACE
