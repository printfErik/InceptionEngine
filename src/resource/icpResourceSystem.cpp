#include "icpResourceSystem.h"
#include "../mesh/icpMeshResource.h"
#include "../render/icpImageResource.h"
#include "../core/icpSystemContainer.h"
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYOBJLOADER_USE_MAPBOX_EARCUT
#define TINYOBJLOADER_IMPLEMENTATION 
#include <tiny_obj_loader.h>

#include "../core/icpConfigSystem.h"

#include <unordered_map>

INCEPTION_BEGIN_NAMESPACE

icpResourceSystem::icpResourceSystem()
{
	/* Two squads for testing
	std::shared_ptr<icpResourceBase> simpleTri = std::make_shared<icpMeshResource>();
	
	icpMeshData meshData;
	meshData.m_vertices.push_back({ {-0.5f, -0.5f, 0.0f}, { 1.0f, 0.0f, 0.0f }, {1.f, 0.f} });
	meshData.m_vertices.push_back({ {0.5f, -0.5f, 0.0f}, { 0.0f, 1.0f, 0.0f },{0.f, 0.f} });
	meshData.m_vertices.push_back({ {0.5f, 0.5f, 0.0f}, { 0.0f, 0.0f, 1.0f }, {0.f, 1.f} });
	meshData.m_vertices.push_back({ {-0.5f, 0.5f, 0.0f}, { 0.0f, 0.0f, 1.0f }, {1.f, 1.f} });

	meshData.m_vertices.push_back({ {-0.5f, -0.5f, -0.5f}, { 1.0f, 0.0f, 0.0f }, {1.f, 0.f} });
	meshData.m_vertices.push_back({ {0.5f, -0.5f, -0.5f}, { 0.0f, 1.0f, 0.0f },{0.f, 0.f} });
	meshData.m_vertices.push_back({ {0.5f, 0.5f, -0.5f}, { 0.0f, 0.0f, 1.0f }, {0.f, 1.f} });
	meshData.m_vertices.push_back({ {-0.5f, 0.5f, -0.5f}, { 0.0f, 0.0f, 1.0f }, {1.f, 1.f} });

	meshData.m_vertexIndices = { 0, 1, 2, 2, 3, 0, 4, 5, 6, 6, 7, 4 };

	auto meshP = std::dynamic_pointer_cast<icpMeshResource>(simpleTri);
	meshP->m_meshData = meshData;
	m_resources.m_allResources["firstTriangle"] = simpleTri;
	*/
}

icpResourceSystem::~icpResourceSystem()
{

}

void icpResourceSystem::loadImageResource(const std::filesystem::path& imgPath)
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

	auto resName = imgPath.stem().string();
	resName = resName + "_img";
	m_resources.m_allResources[resName] = imgRes;

	stbi_image_free(img);
}

void icpResourceSystem::loadObjModelResource(const std::filesystem::path& objPath)
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

			if (uniqueVexticesMap.count(v) == 0)
			{
				uniqueVexticesMap[v] = static_cast<uint32_t>(mesh.m_vertices.size());
				mesh.m_vertices.push_back(v);
			}
			mesh.m_vertexIndices.push_back(uniqueVexticesMap[v]);
		}
	}
	std::shared_ptr<icpResourceBase> simpleTri = std::make_shared<icpMeshResource>();
	auto meshP = std::dynamic_pointer_cast<icpMeshResource>(simpleTri);
	meshP->m_meshData = mesh;

	m_resources.m_allResources[objName] = simpleTri;
}


INCEPTION_END_NAMESPACE

