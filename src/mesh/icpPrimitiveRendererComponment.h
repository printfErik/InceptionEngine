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

	void fillInPrimitiveData(const glm::vec3& color);

	void createTextureImages();
	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipmapLevel);
	void copyBuffer2Image(VkBuffer srcBuffer, VkImage dstImage, uint32_t width, uint32_t height);

	void createTextureImageViews();
	void createTextureSampler();

	std::vector<VkDescriptorSet> m_descriptorSets;

	std::vector<VkBuffer> m_uniformBuffers;
	std::vector<VkDeviceMemory> m_uniformBufferMem;

	VkBuffer m_vertexBuffer{VK_NULL_HANDLE};
	VkBuffer m_indexBuffer{ VK_NULL_HANDLE };
	VkDeviceMemory m_vertexBufferMem{ VK_NULL_HANDLE };
	VkDeviceMemory m_indexBufferMem{ VK_NULL_HANDLE };

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