#pragma once

#include "../core/icpMacros.h"
#include "../core/icpGuid.h"
#include "icpMeshData.h"

#include "../scene/icpComponent.h"
#include <vulkan/vulkan.hpp>
#include "../render/RHI/Vulkan/vk_mem_alloc.h"
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

	void createVertexBuffers();
	void createIndexBuffers();
	void createUniformBuffers();

	void allocateDescriptorSets();

	void fillInPrimitiveData(const glm::vec3& color);

	void createTextureImages();

	void createTextureImageViews();
	void createTextureSampler();

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

	// just one pixel texture as empty texture
	// this method will be slower than creating an another pipeline
	// so todo: use different pipelines
	VkImage m_textureImage;
	VmaAllocation m_textureBufferAllocation;
	VkImageView m_textureImageView;
	VkSampler m_textureSampler;

	std::vector<std::shared_ptr<icpMaterialTemplate>> m_vMaterials;

private:

};


INCEPTION_END_NAMESPACE