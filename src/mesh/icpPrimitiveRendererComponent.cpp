#include "icpPrimitiveRendererComponent.h"
#include "../core/icpSystemContainer.h"
#include "../core/icpLogSystem.h"
#include "../render/icpRenderSystem.h"
#include "../mesh/icpMeshData.h"
#include "../render/RHI/icpGPUBuffer.h"

#include <cmath>
#include <cstring>

INCEPTION_BEGIN_NAMESPACE
void icpPrimitiveRendererComponent::FillInPrimitiveData(const glm::vec3& color)
{
	switch (m_primitive)
	{
	case ePrimitiveType::CUBE:
	{
		std::vector<icpVertex> cubeVertices{
		{{-1,1,1}, color ,{-1, 1, 1 }, {0,0}},
		{{1,1,1},color,{1, 1, 1 }, {0,1}},
		{{1,1,-1},color,{1,1,-1},{1,1}},
		{{-1,1,-1},color,{-1,1,-1},{1,0}},
		{{-1,-1,1},color,{-1,-1,1},{0,1}},
		{{1,-1,1},color,{1,-1,1},{1,1}},
		{{1,-1,-1},color,{1,-1,-1},{0,0}},
		{{-1,-1,-1},color,{-1,-1,-1},{0,1}},

		{ {1,1,1}, color ,{1, 1, 1 }, {0,0} },
		{ {-1,1,1},color,{-1, 1, 1 }, {0,1} },
		{ {-1,-1,1},color,{-1,-1,1},{1,1} },
		{ {1,-1,1},color,{1,-1,1},{1,0} },
		{ {1,1,-1}, color ,{1, 1, -1 }, {1,0} },
		{ {-1,1,-1},color,{-1, 1, -1 }, {0,0} },
		{ {-1,-1,-1},color,{-1,-1,-1},{0,1} },
		{ {1,-1,-1},color,{1,-1,-1},{1,1} },

		{ {1,1,1}, color ,{1, 1, 1 }, {0,0} },
		{ {1,-1,1},color,{-1, -1, 1 }, {0,1} },
		{ {1,-1,-1},color,{-1,-1,-1},{1,1} },
		{ {1,1,-1},color,{1,1,-1},{1,0} },
		{ {-1,1,1}, color ,{-1, 1, 1 }, {0,1} },
		{ {-1,-1,1},color,{-1, -1, 1 }, {0,0} },
		{ {-1,-1,-1},color,{-1,-1,-1},{1,0} },
		{ {1,1,-1},color,{1,1,-1},{1,1} },
		};

		m_vertices.assign(cubeVertices.begin(), cubeVertices.end());

		std::vector<uint32_t> cubeIndex{
			0, 1, 2, 2, 3, 1, 4, 7, 6, 4, 6, 5, 8, 9, 10, 8, 10, 11, 12, 15, 14, 12, 14, 13, 16, 17, 18, 16, 18, 19, 20, 23, 22, 20, 22, 21
		};

		m_vertexIndices.assign(cubeIndex.begin(), cubeIndex.end());
	}
	break;
	case ePrimitiveType::SPHERE:
	{
		const unsigned int X_SEGMENTS = 64;
		const unsigned int Y_SEGMENTS = 64;
		const float PI = 3.14159265359f;
		std::vector<icpVertex> sphereVertices;
		for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
		{
			for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
			{
				float xSegment = (float)x / (float)X_SEGMENTS;
				float ySegment = (float)y / (float)Y_SEGMENTS;
				float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
				float yPos = std::cos(ySegment * PI);
				float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

				sphereVertices.push_back({ {xPos, yPos, zPos}, color, {xPos, yPos, zPos}, {xSegment, ySegment} });
			}
		}
		m_vertices.assign(sphereVertices.begin(), sphereVertices.end());
		std::vector<uint32_t> sphereIndex;

		for (unsigned int y = 0; y < Y_SEGMENTS; ++y)
		{
			for (unsigned int x = 0; x < X_SEGMENTS; ++x)
			{
				const uint32_t i0 = y * (X_SEGMENTS + 1) + x;
				const uint32_t i1 = (y + 1) * (X_SEGMENTS + 1) + x;
				const uint32_t i2 = (y + 1) * (X_SEGMENTS + 1) + x + 1;
				const uint32_t i3 = y * (X_SEGMENTS + 1) + x + 1;

				sphereIndex.push_back(i0);
				sphereIndex.push_back(i1);
				sphereIndex.push_back(i2);
				sphereIndex.push_back(i0);
				sphereIndex.push_back(i2);
				sphereIndex.push_back(i3);
			}
		}
		m_vertexIndices.assign(sphereIndex.begin(), sphereIndex.end());
	}
	break;
	default:
	{
		ICP_LOG_WARING("no such primitive");
	}
	break;
	}
}

void icpPrimitiveRendererComponent::CreateVertexBuffers()
{
	auto rhi = g_system_container.m_renderSystem->GetGPUDevice();
	auto bufferSize = sizeof(m_vertices[0]) * m_vertices.size();
	icpRHIBufferDesc desc{};
	desc.size = bufferSize;
	desc.usage = icpBufferUsage::VERTEX | icpBufferUsage::UPLOAD;
	desc.debugName = "PrimitiveVB";
	MeshVB.buffer = rhi->CreateBuffer(desc, m_vertices.data());
	MeshVB.range = bufferSize;
	MeshVB.offset = 0u;
}

void icpPrimitiveRendererComponent::CreateIndexBuffers()
{
	auto rhi = g_system_container.m_renderSystem->GetGPUDevice();
	auto bufferSize = sizeof(m_vertexIndices[0]) * m_vertexIndices.size();
	icpRHIBufferDesc desc{};
	desc.size = bufferSize;
	desc.usage = icpBufferUsage::INDEX | icpBufferUsage::UPLOAD;
	desc.debugName = "PrimitiveIB";
	MeshIB.buffer = rhi->CreateBuffer(desc, m_vertexIndices.data());
	MeshIB.range = bufferSize;
	MeshIB.offset = 0u;
}


void icpPrimitiveRendererComponent::CreateUniformBuffers()
{
	auto gpuDevice = g_system_container.m_renderSystem->GetGPUDevice();
	auto bufferSize = sizeof(UBOMeshRenderResource);

	MeshUBOs.resize(MAX_FRAMES_IN_FLIGHT);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		icpRHIBufferDesc desc{};
		desc.size = bufferSize;
		desc.usage = icpBufferUsage::UNIFORM;
		desc.debugName = "PrimitiveCB";
		MeshUBOs[i].buffer = gpuDevice->CreateBuffer(desc);
		MeshUBOs[i].range = bufferSize;
		MeshUBOs[i].offset = 0u;
	}
}

void icpPrimitiveRendererComponent::AllocateMeshDescriptorSets()
{
}

std::shared_ptr<icpMaterialTemplate> icpPrimitiveRendererComponent::AddMaterial(eMaterialShadingModel shading_model)
{
	auto materialSystem = g_system_container.m_renderSystem->GetMaterialSubSystem();
	auto instance = materialSystem->createMaterialInstance(shading_model);
	m_pMaterial = instance;

	return instance;
}

void icpPrimitiveRendererComponent::UploadMeshCB(const UBOMeshRenderResource& ubo)
{
	auto curFrame = g_system_container.m_renderSystem->GetSceneRenderer()->GetCurrentFrame();

	void* data = MeshUBOs[curFrame].buffer->Map();
	memcpy(data, &ubo, sizeof(UBOMeshRenderResource));
	MeshUBOs[curFrame].buffer->Unmap();
}

void icpPrimitiveRendererComponent::UploadMaterialCB()
{
	auto curFrame = g_system_container.m_renderSystem->GetSceneRenderer()->GetCurrentFrame();

	void* materialData = m_pMaterial->MaterialUBOs[curFrame].buffer->Map();
	memcpy(materialData, m_pMaterial->CheckMaterialDataCache(), sizeof(PBRShaderMaterial));
	m_pMaterial->MaterialUBOs[curFrame].buffer->Unmap();
}


uint32_t icpPrimitiveRendererComponent::GetMeshIndexNum() const
{
	return m_vertexIndices.size();
}


INCEPTION_END_NAMESPACE
