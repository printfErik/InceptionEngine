#pragma once
#include "../core/icpMacros.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <vulkan/vulkan.hpp>
#include <array>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>


INCEPTION_BEGIN_NAMESPACE

class icpImageResource;
enum class ePrimitiveType;

struct icpVertex
{
	glm::vec3 position;
	glm::vec3 color;
	glm::vec3 normal;
	glm::vec2 texCoord;

	static VkVertexInputBindingDescription getBindingDescription();

	static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescription();

	bool operator==(const icpVertex& v1) const
	{
		return position == v1.position
			&& color == v1.color
			&& texCoord == v1.texCoord
			&& normal == v1.normal;
	}
};


class icpMeshData
{
public:
	std::vector<icpVertex> m_vertices;
	std::vector<uint32_t> m_vertexIndices;

	std::shared_ptr<icpImageResource> m_imgRes = nullptr;

	std::string m_meshName;
};

INCEPTION_END_NAMESPACE

namespace std
{
	template<>
	struct hash<Inception::icpVertex>
	{
		size_t operator()(Inception::icpVertex const& vertex) const
		{
			return ((hash<glm::vec3>()(vertex.position) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.texCoord) << 1);
		}
	};
}