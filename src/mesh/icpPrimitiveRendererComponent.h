#pragma once

#include "../core/icpMacros.h"
#include "../core/icpGuid.h"
#include "icpMeshData.h"

#include "../scene/icpComponent.h"
#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>
#include "../render/material/icpMaterial.h"
#include "icpMeshRendererComponent.h"

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
	void AllocateMeshDescriptorSets();

	void FillInPrimitiveData(const glm::vec3& color);

	std::shared_ptr<icpMaterialTemplate> AddMaterial(eMaterialShadingModel shading_model);

	void UploadMeshCB(const UBOMeshRenderResource& ubo);
	void UploadMaterialCB();

	uint32_t GetMeshIndexNum() const;

	std::vector<icpBufferRenderResource> MeshUBOs;

	icpBufferRenderResource MeshVB;
	icpBufferRenderResource MeshIB;

	std::vector<VkDescriptorSet> MeshDSs;

	std::vector<icpVertex> m_vertices;
	std::vector<uint32_t> m_vertexIndices;

	std::shared_ptr<icpMaterialTemplate> m_pMaterial = nullptr;

private:

};


INCEPTION_END_NAMESPACE