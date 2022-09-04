#include "icpResourceSystem.h"
#include "../mesh/icpMeshResource.h"
INCEPTION_BEGIN_NAMESPACE

icpResourceSystem::icpResourceSystem()
{
	std::shared_ptr<icpResourceBase> simpleTri = std::make_shared<icpMeshResource>();
	
	icpMeshData meshData;
	meshData.m_vertices.push_back({ {-0.5f, -0.5f, 0.0f}, { 1.0f, 0.0f, 0.0f } });
	meshData.m_vertices.push_back({ {0.5f, -0.5f, 0.0f}, { 0.0f, 1.0f, 0.0f } });
	meshData.m_vertices.push_back({ {0.5f, 0.5f, 0.0f}, { 0.0f, 0.0f, 1.0f } });
	meshData.m_vertices.push_back({ {-0.5f, 0.5f, 0.0f}, { 0.0f, 0.0f, 1.0f } });

	meshData.m_vertexIndices = { 0, 1, 2, 2, 3, 0 };

	auto meshP = std::dynamic_pointer_cast<icpMeshResource>(simpleTri);
	meshP->m_meshData = meshData;
	m_resources.m_allResources["firstTriangle"] = simpleTri;
}

icpResourceSystem::~icpResourceSystem()
{

}

INCEPTION_END_NAMESPACE

