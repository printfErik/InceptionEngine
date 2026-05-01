#include "icpMeshRendererComponent.h"

#include "../core/icpSystemContainer.h"
#include "../render/icpRenderSystem.h"
#include "../resource/icpResourceSystem.h"
#include "icpMeshResource.h"

#include <cstring>

INCEPTION_BEGIN_NAMESPACE

void icpMeshRendererComponent::prepareRenderResourceForMesh()
{
	createVertexBuffers();
	createIndexBuffers();
	createUniformBuffers();
	AllocateMeshDescriptorSets();
}

void icpMeshRendererComponent::createUniformBuffers()
{
	auto rhi = g_system_container.m_renderSystem->GetGPUDevice();
	const auto perMeshSize = sizeof(UBOMeshRenderResource);
	MeshUBOs.resize(MAX_FRAMES_IN_FLIGHT);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		icpRHIBufferDesc desc{};
		desc.size = perMeshSize;
		desc.usage = icpBufferUsage::UNIFORM;
		desc.debugName = "MeshCB";
		MeshUBOs[i].buffer = rhi->CreateBuffer(desc);
		MeshUBOs[i].range = perMeshSize;
		MeshUBOs[i].offset = 0u;
	}
}

void icpMeshRendererComponent::AllocateMeshDescriptorSets()
{
}

void icpMeshRendererComponent::createVertexBuffers()
{
	auto rhi = g_system_container.m_renderSystem->GetGPUDevice();
	const auto meshRes = std::dynamic_pointer_cast<icpMeshResource>(
		g_system_container.m_resourceSystem->GetResourceContainer()[icpResourceType::MESH][m_meshResId]);
	const auto bufferSize = sizeof(meshRes->m_meshData.m_vertices[0]) * meshRes->m_meshData.m_vertices.size();

	icpRHIBufferDesc desc{};
	desc.size = bufferSize;
	desc.usage = icpBufferUsage::VERTEX | icpBufferUsage::UPLOAD;
	desc.debugName = "MeshVB";
	MeshVB.buffer = rhi->CreateBuffer(desc, meshRes->m_meshData.m_vertices.data());
	MeshVB.range = bufferSize;
	MeshVB.offset = 0u;
}

void icpMeshRendererComponent::createIndexBuffers()
{
	auto rhi = g_system_container.m_renderSystem->GetGPUDevice();
	const auto meshRes = std::dynamic_pointer_cast<icpMeshResource>(
		g_system_container.m_resourceSystem->GetResourceContainer()[icpResourceType::MESH][m_meshResId]);
	const auto bufferSize = sizeof(meshRes->m_meshData.m_vertexIndices[0]) * meshRes->m_meshData.m_vertexIndices.size();

	icpRHIBufferDesc desc{};
	desc.size = bufferSize;
	desc.usage = icpBufferUsage::INDEX | icpBufferUsage::UPLOAD;
	desc.debugName = "MeshIB";
	MeshIB.buffer = rhi->CreateBuffer(desc, meshRes->m_meshData.m_vertexIndices.data());
	MeshIB.range = bufferSize;
	MeshIB.offset = 0u;
	m_meshVertexIndicesNum = static_cast<uint32_t>(meshRes->m_meshData.m_vertexIndices.size());
}

std::shared_ptr<icpMaterialTemplate> icpMeshRendererComponent::addMaterial(eMaterialShadingModel shadingModel)
{
	if (m_pMaterial)
	{
		m_pMaterial.reset();
	}
	auto materialSystem = g_system_container.m_renderSystem->GetMaterialSubSystem();
	auto instance = materialSystem->createMaterialInstance(shadingModel);
	m_pMaterial = instance;
	return instance;
}

void icpMeshRendererComponent::AddMaterial(std::shared_ptr<icpMaterialTemplate> material)
{
	if (m_pMaterial)
	{
		m_pMaterial.reset();
	}
	m_pMaterial = material;
}

void icpMeshRendererComponent::UploadMeshCB(const UBOMeshRenderResource& ubo)
{
	auto curFrame = g_system_container.m_renderSystem->GetSceneRenderer()->GetCurrentFrame();
	void* data = MeshUBOs[curFrame].buffer->Map();
	memcpy(data, &ubo, sizeof(UBOMeshRenderResource));
	MeshUBOs[curFrame].buffer->Unmap();
}

void icpMeshRendererComponent::UploadMaterialCB()
{
	auto curFrame = g_system_container.m_renderSystem->GetSceneRenderer()->GetCurrentFrame();
	void* materialData = m_pMaterial->MaterialUBOs[curFrame].buffer->Map();
	memcpy(materialData, m_pMaterial->CheckMaterialDataCache(), sizeof(PBRShaderMaterial));
	m_pMaterial->MaterialUBOs[curFrame].buffer->Unmap();
}

uint32_t icpMeshRendererComponent::GetMeshIndexNum() const
{
	return m_meshVertexIndicesNum;
}

INCEPTION_END_NAMESPACE
