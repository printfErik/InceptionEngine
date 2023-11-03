#include "icpResourceSystem.h"
#include "../mesh/icpMeshResource.h"
#include "../render/icpImageResource.h"
#include "../core/icpSystemContainer.h"
#include "../core/icpLogSystem.h"

#define TINYOBJLOADER_USE_MAPBOX_EARCUT
#define TINYOBJLOADER_IMPLEMENTATION 
#include <tiny_obj_loader.h>

#include "../core/icpConfigSystem.h"

#include <unordered_map>
#include "icpGLTFLoaderUtil.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tiny_gltf.h>

INCEPTION_BEGIN_NAMESPACE

icpResourceSystem::icpResourceSystem()
{
}

icpResourceSystem::~icpResourceSystem()
{

}

bool icpResourceSystem::Initialize()
{
	enki::TaskSchedulerConfig config;

	config.numTaskThreadsToCreate += 1;

	m_ekScheduler = std::make_unique<enki::TaskScheduler>();
	m_ekScheduler->Initialize(config);

	AsyncLoadFileResourceTask asyncFileLoadTask;
	asyncFileLoadTask.threadNum = m_ekScheduler->GetThreadNum() - 1;
	asyncFileLoadTask.m_pResourceSystem = shared_from_this();

	m_ekScheduler->AddPinnedTask(&asyncFileLoadTask);
}


std::shared_ptr<icpResourceBase> icpResourceSystem::loadImageResource(const std::filesystem::path& imgPath)
{
	int width, height, channel;
	stbi_uc* img = stbi_load(imgPath.string().data(),
		&width, &height, &channel, STBI_rgb_alpha);

	if (!img)
	{
		throw std::runtime_error("failed to load texture!");
	}

	std::shared_ptr<icpImageResource> imgRes = std::make_shared<icpImageResource>();
	imgRes->setImageBuffer(img, width * height * STBI_rgb_alpha, width, height, STBI_rgb_alpha);

	imgRes->m_mipmapLevel = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
	imgRes->m_resType = icpResourceType::TEXTURE;

	const auto& resName = imgPath.stem().string();
	m_resources[icpResourceType::TEXTURE][resName] = imgRes;


	stbi_image_free(img);
	return imgRes;
}

std::shared_ptr<icpResourceBase> icpResourceSystem::loadObjModelResource(const std::filesystem::path& objPath, bool ifLoadRelatedImgRes)
{
	auto objName = objPath.stem().string();
	tinyobj::ObjReader reader;

	if(reader.ParseFromFile(objPath.string()))
	{
		if (!reader.Error().empty())
		{
			throw std::runtime_error("failed to load model from file");
		}
	}

	if (!reader.Warning().empty())
	{
		//std::cout << reader.Warning() << std::endl;
	}

	auto& attrib = reader.GetAttrib();
	auto& shapes = reader.GetShapes();

	icpMeshData mesh;

	std::unordered_map<icpVertex, uint32_t> uniqueVexticesMap;

	for (const auto& shape : shapes)
	{
		for (const auto& index: shape.mesh.indices)
		{
			icpVertex v{};

			v.position = {
				attrib.vertices[3 * static_cast<size_t>(index.vertex_index) + 0],
				attrib.vertices[3 * static_cast<size_t>(index.vertex_index) + 1],
				attrib.vertices[3 * static_cast<size_t>(index.vertex_index) + 2],
			};

			v.texCoord = {
				attrib.texcoords[2 * static_cast<size_t>(index.texcoord_index) + 0],
				1.f - attrib.texcoords[2 * static_cast<size_t>(index.texcoord_index) + 1],
			};

			v.color = { 1.f, 1.f, 1.f };

			v.normal = {
				attrib.normals[3 * static_cast<size_t>(index.texcoord_index) + 0],
				attrib.normals[3 * static_cast<size_t>(index.texcoord_index) + 1],
				attrib.normals[3 * static_cast<size_t>(index.texcoord_index) + 2]
			};

			if (uniqueVexticesMap.count(v) == 0)
			{
				uniqueVexticesMap[v] = static_cast<uint32_t>(mesh.m_vertices.size());
				mesh.m_vertices.push_back(v);
			}
			mesh.m_vertexIndices.push_back(uniqueVexticesMap[v]);
		}
	}
	std::shared_ptr<icpResourceBase> modelRes = std::make_shared<icpMeshResource>();
	auto meshP = std::dynamic_pointer_cast<icpMeshResource>(modelRes);

	meshP->m_meshData = mesh;
	meshP->m_resType = icpResourceType::MESH;
	meshP->m_id = objName;

	m_resources[icpResourceType::MESH][objName] = modelRes;

	if (ifLoadRelatedImgRes)
	{
		auto imgPath = g_system_container.m_configSystem->m_imageResourcePath / (objName + ".png");
		auto imgP = std::dynamic_pointer_cast<icpImageResource>(g_system_container.m_resourceSystem->loadImageResource(imgPath));
		meshP->m_meshData.m_imgRes = imgP;
	}

	return modelRes;
}

bool icpResourceSystem::LoadGLTFResource(const std::filesystem::path& gltfPath)
{
	tinygltf::Model gltfModel;
	tinygltf::TinyGLTF gltfLoader;
	std::string error, warning;
	bool ret = gltfLoader.LoadASCIIFromFile(&gltfModel, &error, &warning, gltfPath.string());

	if (!error.empty())
	{
		ICP_LOG_ERROR("Load gltf error: ", error);
		return false;
	}

	if (!warning.empty())
	{
		ICP_LOG_WARING("Load gltf error: ", warning);
	}

	if (!ret)
	{
		ICP_LOG_ERROR("Failed to Parse gltf");
		return false;
	}

	std::vector<icpSamplerResource> samplers;
	icpGLTFLoaderUtil::LoadGLTFTextureSamplers(gltfModel, samplers);

	std::vector<icpImageResource> images;
	icpGLTFLoaderUtil::LoadGLTFTextures(gltfModel, samplers, images);

	std::vector<std::shared_ptr<icpMaterialInstance>> materials;
	icpGLTFLoaderUtil::LoadGLTFMaterials(gltfModel, images, materials);

	std::vector<std::vector<icpMeshResource>> meshResources;
	icpGLTFLoaderUtil::LoadGLTFMeshs(gltfModel, materials, meshResources);

	icpGLTFLoaderUtil::LoadGLTFScene(gltfModel, meshResources);

	return true;
}

bool icpResourceSystem::RequestAsyncLoadResource(icpResourceType type, const std::filesystem::path& path)
{
	ResourceLoadTask task{};
	task.type = type;
	task.file_path = path;

	m_taskLoadingQueue.push(task);

	return true;
}


void icpResourceSystem::UpdateSystem()
{
	if (!m_taskLoadingQueue.empty())
	{
		auto& task = m_taskLoadingQueue.front();
		switch (task.type)
		{
		case icpResourceType::GLTF:
			{
			LoadGLTFResource(task.file_path);
			break;
			}
		default:
			{
			break;
			}
		}
		m_taskLoadingQueue.pop();
	}
}

icpResourceContainer& icpResourceSystem::GetResourceContainer()
{
	return m_resources;
}


void AsyncLoadFileResourceTask::Execute()
{
	while (m_bExecuting)
	{
		m_pResourceSystem->UpdateSystem();
	}
}


INCEPTION_END_NAMESPACE

