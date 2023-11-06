#include "icpSceneSystem.h"
#include "icpEntity.h"
#include "icpEntityDataComponent.h"
#include "icpXFormComponent.h"
#include "../render/icpCameraSystem.h"
#include "../mesh/icpMeshRendererComponent.h"
#include "../mesh/icpMeshResource.h"
#include "../resource/icpResourceSystem.h"
#include "../core/icpSystemContainer.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>

#include <fstream>
#include <iostream>

#include "../mesh/icpPrimitiveRendererComponent.h"
#include "../render/icpRenderSystem.h"
#include "../render/light/icpLightComponent.h"

INCEPTION_BEGIN_NAMESPACE

void icpSceneSystem::initializeScene(const std::filesystem::path& mapPath)
{
	//loadSceneFromMapPath("E:\\Development\\InceptionEngine\\test\\testflat");
	LoadDefaultScene();
}


std::shared_ptr<icpGameEntity> icpSceneSystem::CreateEntity(const std::string& name, std::shared_ptr<icpGameEntity> parent)
{
	std::shared_ptr<icpGameEntity> entity = std::make_shared<icpGameEntity>(m_registry.create());
	auto&& entityData = entity->installComponent<icpEntityDataComponent>();
	entityData.m_name = name;
	entityData.m_guid = icpGuid();
	auto&& entityXForm = entity->installComponent<icpXFormComponent>();
	if (!parent)
	{
		m_sceneRoots.push_back(entity);
		return entity;
	}

	auto&& parentXForm = parent->accessComponent<icpXFormComponent>();
	
	entityXForm.m_parent = std::make_shared<icpXFormComponent>(parentXForm);
	parentXForm.m_children.push_back(std::make_shared<icpXFormComponent>(entityXForm));

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

	std::vector<flatbuffers::Offset<fb::flatbufferTreeNode>> cameraChildren;
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
	auto textureResId = builder.CreateString("viking_room_img");
	auto meshRendererCompFb = fb::CreateicpMeshRendererComponent(builder, meshResId, textureResId);

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

	std::vector<flatbuffers::Offset<fb::flatbufferTreeNode>> meshChildren;
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
		addRootNodeToHierarchy(root);
	}
}

void icpSceneSystem::addRootNodeToHierarchy(const fb::flatbufferTreeNode* node)
{
	auto entity = createEntityFromMap(node);
	auto entityP = std::make_shared<icpGameEntity>(entity);
	m_sceneRoots.push_back(entityP);

	auto children = node->m_children();

	for (int j = 0; j < children->size(); j++)
	{
		auto childNode = children->Get(j);
		recursiveAddNodeToHierarchy(childNode, entityP);
	}
}


void icpSceneSystem::recursiveAddNodeToHierarchy(const fb::flatbufferTreeNode* node, std::shared_ptr<icpGameEntity> parent)
{
	auto entity = createEntityFromMap(node);
	auto entityP = std::make_shared<icpGameEntity>(entity);

	auto&& xform = entity.accessComponent<icpXFormComponent>();

	xform.m_parent = std::make_shared<icpXFormComponent>(parent->accessComponent<icpXFormComponent>());
	parent->accessComponent<icpXFormComponent>().m_children.push_back(std::make_shared<icpXFormComponent>(xform));

	auto children = node->m_children();
	for (int j = 0; j < children->size(); j++)
	{
		auto childNode = children->Get(j);
		recursiveAddNodeToHierarchy(childNode, entityP);
	}
}


icpGameEntity icpSceneSystem::createEntityFromMap(const fb::flatbufferTreeNode* node)
{
	auto entityFB = node->m_entity();

	icpGameEntity entity;
	entity.InitializeEntity(m_registry.create(), nullptr);

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
			mesh.m_meshResId = meshRenderFB->m_meshResID()->str();
			//mesh.m_texResId = meshRenderFB->m_textureResID()->str();
		}
	}

	return entity;
}


void icpSceneSystem::getRootEntityList(std::vector<std::shared_ptr<icpGameEntity>>& list)
{
	list = m_sceneRoots;
}

void icpSceneSystem::createMeshEntityFromResource(std::shared_ptr<icpResourceBase> meshRes)
{
	icpGameEntity entity;
	entity.InitializeEntity(m_registry.create(), nullptr);

	auto&& entityData = entity.installComponent<icpEntityDataComponent>();
	entityData.m_name = meshRes->m_id;
	entityData.m_guid = icpGuid();

	auto&& xform = entity.installComponent<icpXFormComponent>();
	
	auto&& mesh = entity.installComponent<icpMeshRendererComponent>();

	mesh.m_meshResId = meshRes->m_id;

	mesh.prepareRenderResourceForMesh();
	auto material = mesh.addMaterial(eMaterialShadingModel::UNLIT);

	material->AddTexture("baseColorTexture",{ meshRes->m_id });
	material->SetupMaterialRenderResources();
}

void icpSceneSystem::LoadDefaultScene()
{
	/*
	// plane
	{
		auto planeEntity = CreateEntity("Plane", nullptr);

		auto&& xform = planeEntity->accessComponent<icpXFormComponent>();
		xform.m_translation = glm::vec3(0.f, -5.f, -5.f);
		xform.m_scale = glm::vec3(10, 0.2, 10);

		auto&& plane = planeEntity->installComponent<icpPrimitiveRendererComponent>();

		plane.m_primitive = ePrimitiveType::CUBE;

		plane.FillInPrimitiveData(glm::vec3(1, 0, 1));
		plane.CreateVertexBuffers();
		plane.CreateIndexBuffers();
		plane.CreateUniformBuffers();
		plane.AllocateDescriptorSets();

		auto&& material = plane.AddMaterial(eMaterialShadingModel::DEFAULT_LIT);

		material->AddTexture("rustediron2_basecolor");
		material->AddTexture("rustediron2_metallic");
		material->AddTexture("rustediron2_normal");
		material->AddTexture("rustediron2_roughness");
		material->AddScalaValue({ "Shininess", 0.1f });
		material->SetupMaterialRenderResources();
	}

	// Cube
	{
		auto cubeEntity = CreateEntity("Cube", nullptr);

		auto&& xform = cubeEntity->accessComponent<icpXFormComponent>();
		xform.m_translation = glm::vec3(0.f, 5.f, 0.f);
		xform.m_scale = glm::vec3(2.f, 2.f, 2.f);

		auto&& cube = cubeEntity->installComponent<icpPrimitiveRendererComponent>();

		cube.m_primitive = ePrimitiveType::CUBE;

		cube.FillInPrimitiveData(glm::vec3(1, 0, 1));
		cube.CreateVertexBuffers();
		cube.CreateIndexBuffers();
		cube.CreateUniformBuffers();
		cube.AllocateDescriptorSets();

		auto&& material = cube.AddMaterial(eMaterialShadingModel::DEFAULT_LIT);

		material->AddTexture("rustediron2_basecolor");
		material->AddTexture("rustediron2_metallic");
		material->AddTexture("rustediron2_normal");
		material->AddTexture("rustediron2_roughness");
		material->AddScalaValue({ "Shininess", 0.1f });
		material->SetupMaterialRenderResources();
	}
	*/
	
	// Sphere
	{
		auto sphereEntity = CreateEntity("Sphere", nullptr);

		auto&& xform = sphereEntity->accessComponent<icpXFormComponent>();
		xform.m_translation = glm::vec3(0.f, 3.f, 0.f);
		xform.m_scale = glm::vec3(1.f, 1.f, 1.f);

		auto&& sphere = sphereEntity->installComponent<icpPrimitiveRendererComponent>();

		sphere.m_primitive = ePrimitiveType::SPHERE;

		sphere.FillInPrimitiveData(glm::vec3(1, 0, 1));
		sphere.CreateVertexBuffers();
		sphere.CreateIndexBuffers();
		sphere.CreateUniformBuffers();
		sphere.AllocateDescriptorSets();

		auto&& material = sphere.AddMaterial(eMaterialShadingModel::PBR_LIT);

		material->AddTexture("baseColorTexture", { "rustediron2_basecolor" });
		material->AddTexture("metallicTexture", { "rustediron2_metallic" });
		material->AddTexture("normalTexture", { "rustediron2_normal" });
		material->AddTexture("roughnessTexture", { "rustediron2_roughness" });
		material->SetupMaterialRenderResources();
	}
	
	// Directional Light 
	{
		auto lightEntity = CreateEntity("DirectionalLight", nullptr);

		auto&& xform = lightEntity->accessComponent<icpXFormComponent>();
		xform.m_translation = glm::vec3(0.f);

		auto&& lightComp = lightEntity->installComponent<icpDirectionalLightComponent>();
		lightComp.m_direction = glm::vec3(1.f, 1.f, 1.f);
		lightComp.m_color = glm::vec3(10.f, 10.f, 10.f);
	}
}

std::shared_ptr<icpGameEntity> icpSceneSystem::FindEntity(icpGuid guid)
{
	
}


INCEPTION_END_NAMESPACE