#pragma once

#include "../core/icpMacros.h"
#include "../core/icpGuid.h"
#include "icpMeshData.h"

#include "../scene/icpComponent.h"
#include <vulkan/vulkan.hpp>

INCEPTION_BEGIN_NAMESPACE

enum class ePrimitiveType
{
	CUBE = 0,
	SPHERE = 1,
	NONE = 9999
};

class icpPrimitiveRendererComponment : public icpComponentBase
{
public:
	icpPrimitiveRendererComponment() = default;
	virtual ~icpPrimitiveRendererComponment() = default;

	ePrimitiveType m_primitive = ePrimitiveType::NONE;

	void createVertexBuffers();
	void createIndexBuffers();
	void createUniformBuffers();

	void allocateDescriptorSets();

	void fillInPrimitiveData();

	std::vector<VkDescriptorSet> m_descriptorSets;

	std::vector<VkBuffer> m_uniformBuffers;
	std::vector<VkDeviceMemory> m_uniformBufferMem;

	VkBuffer m_vertexBuffer{VK_NULL_HANDLE};
	VkBuffer m_indexBuffer{ VK_NULL_HANDLE };
	VkDeviceMemory m_vertexBufferMem{ VK_NULL_HANDLE };
	VkDeviceMemory m_indexBufferMem{ VK_NULL_HANDLE };

	std::vector<icpVertex> m_vertices;
	std::vector<uint32_t> m_vertexIndices;

private:

};


INCEPTION_END_NAMESPACE