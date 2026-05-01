#include "icpMeshData.h"

INCEPTION_BEGIN_NAMESPACE

std::vector<icpVertexAttributeDesc> icpVertex::getAttributeDescription()
{
	return {
		{ 0, icpFormat::R32G32B32_FLOAT, offsetof(icpVertex, position) },
		{ 1, icpFormat::R32G32B32_FLOAT, offsetof(icpVertex, color) },
		{ 2, icpFormat::R32G32B32_FLOAT, offsetof(icpVertex, normal) },
		{ 3, icpFormat::R32G32_FLOAT, offsetof(icpVertex, texCoord) },
	};
}

INCEPTION_END_NAMESPACE
