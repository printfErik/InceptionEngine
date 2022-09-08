#include "icpResourceSystem.h"
#include "../mesh/icpMeshResource.h"
#include "../render/icpImageResource.h"
#include "../core/icpSystemContainer.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "../core/icpConfigSystem.h"

INCEPTION_BEGIN_NAMESPACE
	icpResourceSystem::icpResourceSystem()
{
	std::shared_ptr<icpResourceBase> simpleTri = std::make_shared<icpMeshResource>();
	
	icpMeshData meshData;
	meshData.m_vertices.push_back({ {-1.f, -1.f, 0.0f}, { 1.0f, 0.0f, 0.0f }, {1.f, 0.f} });
	meshData.m_vertices.push_back({ {1.f, -1.f, 0.0f}, { 0.0f, 1.0f, 0.0f }, {0.f, 0.f} });
	meshData.m_vertices.push_back({ {1.f, 1.f, 0.0f}, { 0.0f, 0.0f, 1.0f }, {0.f, 1.f} });
	meshData.m_vertices.push_back({ {-1.f, 1.f, 0.0f}, { 0.0f, 0.0f, 1.0f }, {1.f, 1.f} });

	meshData.m_vertexIndices = { 0, 1, 2, 2, 3, 0 };

	auto meshP = std::dynamic_pointer_cast<icpMeshResource>(simpleTri);
	meshP->m_meshData = meshData;
	m_resources.m_allResources["firstTriangle"] = simpleTri;
}

icpResourceSystem::~icpResourceSystem()
{

}

void icpResourceSystem::loadImageResource(const std::filesystem::path& imgPath)
{
	int width, height, channel;
	stbi_uc* img = stbi_load(imgPath.string().c_str(),
		&width, &height, &channel, STBI_rgb_alpha);

	if (!img)
	{
		throw std::runtime_error("failed to load texture!");
	}

	std::shared_ptr<icpImageResource> imgRes = std::make_shared<icpImageResource>();
	imgRes->setImageBuffer(img, width * height * STBI_rgb_alpha, width, height, STBI_rgb_alpha);

	m_resources.m_allResources["superman"] = imgRes;

	stbi_image_free(img);
}

INCEPTION_END_NAMESPACE

