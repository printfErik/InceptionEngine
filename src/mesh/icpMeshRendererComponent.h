#pragma once

#include "../core/icpMacros.h"
#include "../core/icpGuid.h"

#include "../scene/icpComponent.h"

#include <vulkan/vulkan.hpp>

INCEPTION_BEGIN_NAMESPACE

class icpImageResource;

class icpMeshRendererComponent : public icpComponentBase
{
public:
	icpMeshRendererComponent() = default;
	virtual ~icpMeshRendererComponent() = default;

	void prepareRenderResourceForMesh();
	void createVertexBuffers();
	void createIndexBuffers();
	void createUniformBuffers();

	void allocateDescriptorSets();

	void createTextureImages();
	void createTextureImageViews(size_t mipmaplevel);
	void createTextureSampler();

	std::vector<VkDescriptorSet> m_descriptorSetsPerFrame;
	std::vector<VkDescriptorSet> m_descriptorSetsPerMaterial;
	std::vector<VkDescriptorSet> m_descriptorSetsPerObject;

	std::vector<VkBuffer> m_perMaterialUniformBuffers;
	std::vector<VkDeviceMemory> m_perMaterialUniformBufferMem;

	std::vector<VkBuffer> m_perFrameStorageBuffers;
	std::vector<VkDeviceMemory> m_perFrameStorageBufferMem;

	std::vector<VkBuffer> m_objectStorageBuffers;
	std::vector<VkDeviceMemory> m_objectStorageBufferMem;

	VkBuffer m_vertexBuffer;
	VkBuffer m_indexBuffer;
	VkDeviceMemory m_vertexBufferMem;
	VkDeviceMemory m_indexBufferMem;

	VkImage m_textureImage;
	VkDeviceMemory m_textureBufferMem;
	VkImageView m_textureImageView;
	VkSampler m_textureSampler;

	std::shared_ptr<icpImageResource> m_imgRes = nullptr;

	std::string m_meshResId;
	std::string m_texResId;
};


INCEPTION_END_NAMESPACE