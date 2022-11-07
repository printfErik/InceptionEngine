#include "icpSceneSystem.h"
#include "icpEntity.h"
#include "icpEntityDataComponent.h"
#include "icpXFormComponent.h"
#include "../render/icpCameraSystem.h"
#include "../mesh/icpMeshRendererComponent.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>

#include <fstream>
#include <iostream>

INCEPTION_BEGIN_NAMESPACE

void icpSceneSystem::initializeScene(const std::filesystem::path& mapPath)
{
	loadSceneFromMapPath(mapPath);

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

	// camera node 
	auto cameraGuidfbb = fb::CreateicpGuid(builder, icpGuid());
	auto cameraName = builder.CreateString("camera");
	auto cameraEntityDataComp = fb::CreateicpEntityDataComponent(builder, cameraName, cameraGuidfbb);

	auto cameraPosition = fb::CreateVector3(builder, 0.f, 0.f, 5.f);
	auto cameraQua = glm::qua<float>(1.f, 0.f, 0.f, 0.f);
	auto cameraQuaFb = fb::CreateQuaternion(builder, cameraQua.w, cameraQua.x, cameraQua.y, cameraQua.z);
	auto euler = glm::eulerAngles(cameraQua);
	auto camerarotation = fb::CreateVector3(builder, euler.x, euler.y, euler.z);
	auto camerascale = fb::CreateVector3(builder, 1.f, 1.f, 1.f);
	auto cameraXformComp = fb::CreateicpXFormComponent(builder, cameraPosition, camerarotation, cameraQuaFb, camerascale);

	auto cameraClearColorFb = fb::CreateVector3(builder, 0.f, 0.f, 0.f);
	float cameraFov = glm::radians(45.f);
	float near = 0.1f;
	float far = 100.f;
	float cameraSpeed = 0.001f;
	float cameraRotSpeed = glm::radians(0.1f);
	auto viewDirFB = fb::CreateVector3(builder, 0.f, 0.f, -1.f);
	auto upDirFb = fb::CreateVector3(builder, 0.f, 1.f, 0.f);
	auto cameraComp = fb::CreateicpCameraComponent(builder, cameraClearColorFb, cameraFov, near, far, cameraSpeed, cameraRotSpeed, viewDirFB, upDirFb);

	std::vector<uint8_t> componentsType;
	std::vector<flatbuffers::Offset<void>> compUnions;

	componentsType.push_back(fb::icpComponentBase_icpEntityDataComponent);
	compUnions.push_back(cameraEntityDataComp.Union());

	componentsType.push_back(fb::icpComponentBase_icpXFormComponent);
	compUnions.push_back(cameraXformComp.Union());

	componentsType.push_back(fb::icpComponentBase_icpCameraComponent);
	compUnions.push_back(cameraComp.Union());

	auto typefb = builder.CreateVector(componentsType);
	auto unionfb = builder.CreateVector(compUnions);

	auto CameraEntityfb = fb::CreateflatbufferEntity(builder, typefb, unionfb);

	std::vector<flatbuffers::Offset<fb::flatbufferEntity>> cameraChildren;
	auto cameraChildrenFb = builder.CreateVector(cameraChildren);

	auto CameraTreeNode = fb::CreateflatbufferTreeNode(builder, CameraEntityfb, cameraChildrenFb);

	// mesh node
	auto meshGuidfbb = fb::CreateicpGuid(builder, icpGuid());
	auto meshName = builder.CreateString("Mesh");
	auto meshEntityDataComp = fb::CreateicpEntityDataComponent(builder, meshName, meshGuidfbb);

	auto meshPosition = fb::CreateVector3(builder, 0.f, 0.f, 0.f);
	auto meshEuler = glm::vec3(glm::radians(-90.0f), 0.f, glm::radians(-90.0f));

	auto meshQua = glm::qua<float>(meshEuler);
	auto meshQuaFb = fb::CreateQuaternion(builder, meshQua.w, meshQua.x, meshQua.y, meshQua.z);

	auto meshRotation = fb::CreateVector3(builder, meshEuler.x, meshEuler.y, meshEuler.z);
	auto meshscale = fb::CreateVector3(builder, 1.f, 1.f, 1.f);
	auto meshXformComp = fb::CreateicpXFormComponent(builder, meshPosition, meshRotation, meshQuaFb, meshscale);

	auto meshResId = builder.CreateString("viking_room");
	auto meshRendererCompFb = fb::CreateicpMeshRendererComponent(builder, meshResId);

	std::vector<uint8_t> meshComponentsType;
	std::vector<flatbuffers::Offset<void>> meshCompUnions;

	meshComponentsType.push_back(fb::icpComponentBase_icpEntityDataComponent);
	meshCompUnions.push_back(meshEntityDataComp.Union());

	meshComponentsType.push_back(fb::icpComponentBase_icpXFormComponent);
	meshCompUnions.push_back(meshXformComp.Union());

	meshComponentsType.push_back(fb::icpComponentBase_icpMeshRendererComponent);
	meshCompUnions.push_back(meshRendererCompFb.Union());

	auto meshTypefb = builder.CreateVector(meshComponentsType);
	auto meshUnionfb = builder.CreateVector(meshCompUnions);

	auto meshEntityfb = fb::CreateflatbufferEntity(builder, meshTypefb, meshUnionfb);

	std::vector<flatbuffers::Offset<fb::flatbufferEntity>> meshChildren;
	auto meshChildrenFb = builder.CreateVector(meshChildren);

	auto meshTreeNode = fb::CreateflatbufferTreeNode(builder, meshEntityfb, meshChildrenFb);

	std::vector<flatbuffers::Offset<fb::flatbufferTreeNode>> rootNodes;
	rootNodes.push_back(CameraTreeNode);
	rootNodes.push_back(meshTreeNode);

	auto rootNodesFb = builder.CreateVector(rootNodes);

	auto firstScene = fb::CreateflatbufferScene(builder, firstSceneName, rootNodesFb);

	builder.Finish(firstScene);

	uint8_t* buf = builder.GetBufferPointer();
	int size = builder.GetSize(); 

	std::ofstream outFile;
	outFile.open(outPath, std::ios::out | std::ios::binary);

	outFile.write((char*)buf, size);
	outFile.close();
}

void icpSceneSystem::loadSceneFromMapPath(const std::filesystem::path& mapPath)
{
	std::ifstream inFile(mapPath, std::ios::in | std::ios::binary);
	if (!inFile)
	{
		std::cout << "read map error" << std::endl;
		return;
	}

	auto fileSize = std::filesystem::file_size(mapPath);

	const std::vector<uint8_t> fileContent(fileSize, '\0');

	inFile.read((char*)fileContent.data(), fileSize);

	auto sceneInfo = fb::GetflatbufferScene(fileContent.data());

	auto sceneName = sceneInfo->m_name()->str();
	auto rootNodes = sceneInfo->m_sceneRoot();

	auto rootSize = rootNodes->size();
	for (int i = 0; i < rootSize; i++)
	{
		auto root = rootNodes->Get(i);
		addRootNodeToHierachy(root);
	}
}

void icpSceneSystem::addRootNodeToHierachy(const fb::flatbufferTreeNode* node)
{
	auto entityFB = node->m_entity();

	icpGameEntity entity;
	entity.initializeEntity(m_registry.create(), this);

	auto compUnionTypes = entityFB->m_components_type();
	auto comps = entityFB->m_components();

	for (int i = 0; i < compUnionTypes->size(); i++)
	{
		auto compT = compUnionTypes->Get(i);
		auto comp = comps->Get(i);
		if (compT == fb::icpComponentBase_icpEntityDataComponent)
		{
			auto entityDataFB = static_cast<const fb::icpEntityDataComponent*>(comp);
			auto&& entityData = entity.installComponent<icpEntityDataComponent>();
			entityData.m_name = entityDataFB->m_name()->str();
			entityData.m_guid = icpGuid(entityDataFB->m_guid()->m_guid());
		}
		else if (compT == fb::icpComponentBase_icpXFormComponent)
		{
			auto XformFB = static_cast<const fb::icpXFormComponent*>(comp);
			auto&& xform = entity.installComponent<icpXFormComponent>();
			xform.m_translation = glm::vec3(XformFB->m_translation()->x(), XformFB->m_translation()->y(), XformFB->m_translation()->z());
			xform.m_rotation = glm::vec3(XformFB->m_rotation()->x(), XformFB->m_rotation()->y(), XformFB->m_rotation()->z());
			xform.m_scale = glm::vec3(XformFB->m_scale()->x(), XformFB->m_scale()->y(), XformFB->m_scale()->z());
			xform.m_quternionRot = glm::qua<float>(XformFB->m_quternionRot()->w(), XformFB->m_quternionRot()->x(), XformFB->m_scale()->y(), XformFB->m_scale()->z());
		}
		else if (compT == fb::icpComponentBase_icpCameraComponent)
		{
			auto cameraFB = static_cast<const fb::icpCameraComponent*>(comp);
			auto&& camera = entity.installComponent<icpCameraComponent>();
			camera.m_clearColor = glm::vec3(cameraFB->m_clearColor()->x(), cameraFB->m_clearColor()->y(), cameraFB->m_clearColor()->z());
			camera.m_fov = cameraFB->m_fov();
			camera.m_near = cameraFB->m_near();
			camera.m_far = cameraFB->m_far();
			camera.m_cameraSpeed = cameraFB->m_cameraSpeed();
			camera.m_cameraRotationSpeed = cameraFB->m_cameraRotationSpeed();
			camera.m_viewDir = glm::vec3(cameraFB->m_viewDir()->x(), cameraFB->m_viewDir()->y(), cameraFB->m_viewDir()->z());
			camera.m_upDir = glm::vec3(cameraFB->m_upDir()->x(), cameraFB->m_upDir()->y(), cameraFB->m_upDir()->z());
		}
		else if (compT == fb::icpComponentBase_icpMeshRendererComponent)
		{
			auto meshRenderFB = static_cast<const fb::icpMeshRendererComponent*>(comp);
			auto&& mesh = entity.installComponent<icpMeshRendererComponent>();
			mesh.m_meshResId = meshRenderFB->m_resId()->str();
		}
	}

	auto children = node->m_children();
}




INCEPTION_END_NAMESPACE