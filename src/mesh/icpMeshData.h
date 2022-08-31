#pragma once
#include "core/icpMacros.h"
#include <glm/glm.hpp>
#include <vulkan/>

INCEPTION_BEGIN_NAMESPACE

class icpMeshData
{
public:
	std::vector<glm::vec3> m_vertices;
	std::vector<glm::vec3> m_colors;

	static VkVertexInputBindingDescription getBindingDescription() 
	{
		VkVertexInputBindingDescription bindingDescription{};
		return bindingDescription;
	}
};


INCEPTION_END_NAMESPACE