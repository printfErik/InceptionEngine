#include "icpMeshData.h"
#include "../core/icpSystemContainer.h"
#include "../render/icpRenderSystem.h"
#include "../render/icpVulkanUtility.h"
#include "../render/icpImageResource.h"
#include "../core/icpLogSystem.h"

INCEPTION_BEGIN_NAMESPACE

VkVertexInputBindingDescription icpVertex::getBindingDescription()
{
	VkVertexInputBindingDescription bindingDescription{};

	bindingDescription.binding = 0;
	bindingDescription.stride = sizeof(icpVertex);
	bindingDescription.inputRate = VkVertexInputRate::VK_VERTEX_INPUT_RATE_VERTEX;

	return bindingDescription;
}

std::array<VkVertexInputAttributeDescription, 4> icpVertex::getAttributeDescription()
{
	std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions{};
	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[0].offset = offsetof(icpVertex, position);

	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[1].offset = offsetof(icpVertex, color);

	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 2;
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[1].offset = offsetof(icpVertex, normal);

	attributeDescriptions[2].binding = 0;
	attributeDescriptions[2].location = 3;
	attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[2].offset = offsetof(icpVertex, texCoord);

	return attributeDescriptions;
}


INCEPTION_END_NAMESPACE