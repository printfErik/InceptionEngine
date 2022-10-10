#include "icpSceneSystem.h"
#include "icpEntity.h"
#include "icpEntityDataComponent.h"
#include "icpXFormComponent.h"

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




INCEPTION_END_NAMESPACE