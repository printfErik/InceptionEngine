#pragma once

#include "../core/icpMacros.h"
#include "../core/icpGuid.h"
#include "icpMeshData.h"

#include "../scene/icpComponent.h"
#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>
#include "../render/material/icpMaterial.h"

INCEPTION_BEGIN_NAMESPACE

enum class ePrimitiveType
{
	CUBE = 0,
	SPHERE = 1,
	PRIMITIVE_TYPE_MAX,
};

class icpPrimitiveRendererComponent : public icpComponentBase
{
public:
	icpPrimitiveRendererComponent() = default;
	virtual ~icpPrimitiveRendererComponent() = default;

	ePrimitiveType m_primitive = ePrimitiveType::PRIMITIVE_TYPE_MAX;

	void CreateVertexBuffers();
	void CreateIndexBuffers();
	void CreateUniformBuffers();

	void AllocateDescriptorSets();

	void FillInPrimitiveData(const glm::vec3& color);

	std::shared_ptr<icpMaterialTemplate> AddMaterial(eMaterialShadingModel shading_model);

	std::vector<VkDescriptorSet> m_descriptorSets;

	std::vector<VkBuffer> m_uniformBuffers;
	std::vector<VmaAllocation> m_uniformBufferAllocations;

	VkBuffer m_vertexBuffer{VK_NULL_HANDLE};
	VmaAllocation m_vertexBufferAllocation{ VK_NULL_HANDLE };

	VkBuffer m_indexBuffer{ VK_NULL_HANDLE };
	VmaAllocation m_indexBufferAllocation{ VK_NULL_HANDLE };

	std::vector<icpVertex> m_vertices;
	std::vector<uint32_t> m_vertexIndices;

	std::vector<std::shared_ptr<icpMaterialTemplate>> m_vMaterials;

private:

};


INCEPTION_END_NAMESPACE