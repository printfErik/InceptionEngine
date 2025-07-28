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
#include <vk_mem_alloc.h>

#include "../render/RHI/icpGPUBuffer.h"

INCEPTION_BEGIN_NAMESPACE
	class icpImageResource;


struct UBOMeshRenderResource
{
	glm::mat4 model;
	glm::mat4 normalMatrix;
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

	void AddMaterial(std::shared_ptr<icpMaterialTemplate> material);
	std::shared_ptr<icpMaterialTemplate> addMaterial(eMaterialShadingModel shading_model);

	void UploadMeshCB(const UBOMeshRenderResource& ubo);
	void UploadMaterialCB();

	std::vector<icpBufferRenderResource> MeshUBOs;

	icpBufferRenderResource MeshVB;
	icpBufferRenderResource MeshIB;

	std::string m_meshResId;

	uint32_t m_meshVertexIndicesNum = 0;

	// only one material
	std::shared_ptr<icpMaterialTemplate> m_pMaterial = nullptr;
};


INCEPTION_END_NAMESPACE