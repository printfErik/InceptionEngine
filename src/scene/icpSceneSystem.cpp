#include "icpSceneSystem.h"
#include "icpEntity.h"
#include "icpEntityDataComponent.h"
#include "icpXFormComponent.h"

#include "../fb_gen/Scene_generated.h"
#include "../fb_gen/component_generated.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>

INCEPTION_BEGIN_NAMESPACE

void icpSceneSystem::initializeScene(const std::filesystem::path& mapPath)
{
	
}

icpGameEntity icpSceneSystem::createEntity(const std::string& name)
{
	icpGameEntity entity;
	entity.initializeEntity(m_registry.create(), this);
	auto&& entityData = entity.installComponent<icpEntityDataComponent>();
	entityData.m_name = name;
	entityData.m_guid = icpGuid();

	auto&& entityXForm = entity.installComponent<icpXFormComponent>();


	return entity;
}

void icpSceneSystem::saveScene(const std::filesystem::path& outPath)
{
	flatbuffers::FlatBufferBuilder builder(1024);
	auto firstSceneName = builder.CreateString("firstScene");

	auto icpGuidfbb = fb::CreateicpGuid(builder, icpGuid());

	auto meshName = builder.CreateString("mesh");

	auto entityDataComp = fb::CreateicpEntityDataComponent(builder, meshName, icpGuidfbb);

	auto position = fb::CreateVector3(builder, 0.f, 0.f, 0.5f);

	auto qua = glm::qua<float>(1.f, 0.f, 0.f, 0.f);

	auto quaFb = fb::CreateQuaternion(builder, qua.w, qua.x, qua.y, qua.z);

	auto euler = glm::eulerAngles(qua);

	auto rotation = fb::CreateVector3(builder, euler.x, euler.y, euler.z);
	auto scale = fb::CreateVector3(builder, 1.f, 1.f, 1.f);

	auto xformComp = fb::CreateicpXFromComponent(builder, position, rotation, quaFb, scale);

	std::vector<uint8_t> componentsType;
	std::vector<flatbuffers::Offset<void>> compUnions;

	componentsType.push_back(fb::icpComponentBase_icpEntityDataComponent);
	compUnions.push_back(entityDataComp.Union());

	componentsType.push_back(fb::icpComponentBase_icpXFromComponent);
	compUnions.push_back(xformComp.Union());

	auto typefb = builder.CreateVector(componentsType);
	auto unionfb = builder.CreateVector(compUnions);

	auto entityfb = fb::CreateflatbufferEntity(builder, typefb, unionfb);

}




INCEPTION_END_NAMESPACE