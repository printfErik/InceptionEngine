#pragma once

#include "../core/icpMacros.h"
#include "../core/icpGuid.h"

#include "../scene/icpComponent.h"

#include <vulkan/vulkan.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../render/material/icpMaterial.h"
#include "../render/RHI/Vulkan/vk_mem_alloc.h"

INCEPTION_BEGIN_NAMESPACE

class icpImageResource;


struct UBOMeshRenderResource
{
	glm::mat4 model;
	glm::mat3 normalMatrix;
};


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

	std::shared_ptr<icpMaterialTemplate> addMaterial(eMaterialShadingModel shading_model);

	std::vector<VkDescriptorSet> m_perMeshDSs;

	std::vector<VkBuffer> m_perMeshUniformBuffers;
	std::vector<VmaAllocation> m_perMeshUniformBufferAllocations;

	VkBuffer m_vertexBuffer{ VK_NULL_HANDLE };
	VmaAllocation m_vertexBufferAllocation{ VK_NULL_HANDLE };

	VkBuffer m_indexBuffer{ VK_NULL_HANDLE };
	VmaAllocation m_indexBufferAllocation{ VK_NULL_HANDLE };

	std::string m_meshResId;

	// only one material for one mesh for now
	std::vector<std::shared_ptr<icpMaterialTemplate>> m_materials;
};


INCEPTION_END_NAMESPACE