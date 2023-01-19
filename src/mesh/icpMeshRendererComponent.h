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

INCEPTION_BEGIN_NAMESPACE

class icpImageResource;


struct UBOMeshRenderResource
{
	glm::mat4 model;
};

/* todo maybe later
struct SSBOObjects
{
	UBOMeshRenderResource all_meshes[128];
};
*/

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

	VkDescriptorSet m_perMeshDS{ VK_NULL_HANDLE };

	VkBuffer m_perMeshUniformBuffers{ VK_NULL_HANDLE };
	VkDeviceMemory m_perMeshUniformBufferMem{ VK_NULL_HANDLE };

	VkBuffer m_vertexBuffer{ VK_NULL_HANDLE };
	VkDeviceMemory m_vertexBufferMem{ VK_NULL_HANDLE };

	VkBuffer m_indexBuffer{ VK_NULL_HANDLE };
	VkDeviceMemory m_indexBufferMem{ VK_NULL_HANDLE };

	std::string m_meshResId;

	// only one material for one mesh for now
	std::vector<std::shared_ptr<icpMaterialTemplate>> m_materials;
};


INCEPTION_END_NAMESPACE