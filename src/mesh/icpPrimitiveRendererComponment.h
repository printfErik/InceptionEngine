#pragma once

#include "../core/icpMacros.h"
#include "../core/icpGuid.h"
#include "icpMeshData.h"

#include "../scene/icpComponent.h"
#include <vulkan/vulkan.hpp>
#include "../render/RHI/Vulkan/vk_mem_alloc.h"

INCEPTION_BEGIN_NAMESPACE

enum class ePrimitiveType
{
	CUBE = 0,
	SPHERE = 1,
	PRIMITIVE_TYPE_MAX,
};

class icpPrimitiveRendererComponment : public icpComponentBase
{
public:
	icpPrimitiveRendererComponment() = default;
	virtual ~icpPrimitiveRendererComponment() = default;

	ePrimitiveType m_primitive = ePrimitiveType::PRIMITIVE_TYPE_MAX;

	void createVertexBuffers();
	void createIndexBuffers();
	void createUniformBuffers();

	void allocateDescriptorSets();

	void fillInPrimitiveData(const glm::vec3& color);

	void createTextureImages();

	void createTextureImageViews();
	void createTextureSampler();

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
	// this method will be slower than create an another pipeline
	// so todo: use different pipeline
	VkImage m_textureImage;
	VkDeviceMemory m_textureBufferMem;
	VkImageView m_textureImageView;
	VkSampler m_textureSampler;

private:

};


INCEPTION_END_NAMESPACE