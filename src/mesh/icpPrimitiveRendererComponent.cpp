#include "icpPrimitiveRendererComponent.h"
#include "../core/icpSystemContainer.h"
#include "../core/icpLogSystem.h"
#include "../render/icpRenderSystem.h"
#include "../render/RHI/Vulkan/icpVulkanUtility.h"
#include "../mesh/icpMeshData.h"
#include <vk_mem_alloc.h>
#include "../render/renderPass/icpMainForwardPass.h"
#include "../render/RHI/icpDescirptorSet.h"
#include "../render/RHI/icpGPUBuffer.h"

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

		bool oddRow = false;
		for (unsigned int y = 0; y < Y_SEGMENTS; ++y)
		{
			if (!oddRow) // even rows: y == 0, y == 2; and so on
			{
				for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
				{
					sphereIndex.push_back(y * (X_SEGMENTS + 1) + x);
					sphereIndex.push_back((y + 1) * (X_SEGMENTS + 1) + x);
				}
			}
			else
			{
				for (int x = X_SEGMENTS; x >= 0; --x)
				{
					sphereIndex.push_back((y + 1) * (X_SEGMENTS + 1) + x);
					sphereIndex.push_back(y * (X_SEGMENTS + 1) + x);
				}
			}
			oddRow = !oddRow;
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
	auto gpuDevice = g_system_container.m_renderSystem->GetGPUDevice();

	auto bufferSize = sizeof(m_vertices[0]) * m_vertices.size();

	VkBuffer stagingBuffer;
	VmaAllocation stagingBufferAllocation;

	VkSharingMode mode = 
		gpuDevice->GetQueueFamilyIndices().m_graphicsFamily.value() == gpuDevice->GetQueueFamilyIndices().m_transferFamily.value()
			? VK_SHARING_MODE_EXCLUSIVE
			: VK_SHARING_MODE_CONCURRENT;

	icpVulkanUtility::CreateGPUBuffer(
		bufferSize,
		mode,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		gpuDevice->GetVmaAllocator(),
		stagingBufferAllocation,
		stagingBuffer
	);

	void* data;
	vmaMapMemory(gpuDevice->GetVmaAllocator(), stagingBufferAllocation, &data);
	memcpy(data, m_vertices.data(), (size_t)bufferSize);
	vmaUnmapMemory(gpuDevice->GetVmaAllocator(), stagingBufferAllocation);

	icpVulkanUtility::CreateGPUBuffer(
		bufferSize,
		mode,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		gpuDevice->GetVmaAllocator(),
		m_vertexBufferAllocation,
		m_vertexBuffer
	);

	icpVulkanUtility::copyBuffer(stagingBuffer,
		m_vertexBuffer,
		bufferSize,
		gpuDevice->GetLogicalDevice(),
		gpuDevice->GetTransferCommandPool(),
		gpuDevice->GetTransferQueue()
	);

	vmaDestroyBuffer(gpuDevice->GetVmaAllocator(), stagingBuffer, stagingBufferAllocation);
	
}

void icpPrimitiveRendererComponent::CreateIndexBuffers()
{
	auto gpuDevice = g_system_container.m_renderSystem->GetGPUDevice();
	VkDeviceSize bufferSize = sizeof(m_vertexIndices[0]) * m_vertexIndices.size();

	VkBuffer stagingBuffer{ VK_NULL_HANDLE };
	VmaAllocation stagingBufferAllocation{ VK_NULL_HANDLE };

	VkSharingMode mode = gpuDevice->GetQueueFamilyIndices().m_graphicsFamily.value() == gpuDevice->GetQueueFamilyIndices().m_transferFamily.value() ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;

	icpVulkanUtility::CreateGPUBuffer(
		bufferSize,
		mode,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		gpuDevice->GetVmaAllocator(),
		stagingBufferAllocation,
		stagingBuffer
	);

	void* data;
	vmaMapMemory(gpuDevice->GetVmaAllocator(), stagingBufferAllocation, &data);
	memcpy(data, m_vertexIndices.data(), (size_t)bufferSize);
	vmaUnmapMemory(gpuDevice->GetVmaAllocator(), stagingBufferAllocation);

	icpVulkanUtility::CreateGPUBuffer(
		bufferSize,
		mode,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		gpuDevice->GetVmaAllocator(),
		m_indexBufferAllocation,
		m_indexBuffer
	);

	icpVulkanUtility::copyBuffer(stagingBuffer,
		m_indexBuffer,
		bufferSize,
		gpuDevice->GetLogicalDevice(),
		gpuDevice->GetTransferCommandPool(),
		gpuDevice->GetTransferQueue()
	);

	vmaDestroyBuffer(gpuDevice->GetVmaAllocator(), stagingBuffer, stagingBufferAllocation);
}

void icpPrimitiveRendererComponent::AllocateDescriptorSets()
{
	auto gpuDevice = g_system_container.m_renderSystem->GetGPUDevice();

	icpDescriptorSetCreation creation{};
	auto layout = g_system_container.m_renderSystem->GetRenderPassManager()
		->accessRenderPass(eRenderPass::MAIN_FORWARD_PASS)
		->m_DSLayouts[icpMainForwardPass::eMainForwardPassDSType::PER_MESH];

	creation.layoutInfo = layout;

	std::vector<icpBufferRenderResourceInfo> bufferInfos;
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		icpBufferRenderResourceInfo bufferInfo{};
		bufferInfo.buffer = m_uniformBuffers[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UBOMeshRenderResource);
		bufferInfos.push_back(bufferInfo);
	}

	creation.SetUniformBuffer(0, bufferInfos);
	gpuDevice->CreateDescriptorSet(creation, m_descriptorSets);
}

void icpPrimitiveRendererComponent::CreateUniformBuffers()
{
	auto gpuDevice = g_system_container.m_renderSystem->GetGPUDevice();
	auto bufferSize = sizeof(UBOMeshRenderResource);

	m_uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	m_uniformBufferAllocations.resize(MAX_FRAMES_IN_FLIGHT);

	VkSharingMode mode = gpuDevice->GetQueueFamilyIndices().m_graphicsFamily.value() == gpuDevice->GetQueueFamilyIndices().m_transferFamily.value() ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		icpVulkanUtility::CreateGPUBuffer(
			bufferSize,
			mode,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			gpuDevice->GetVmaAllocator(),
			m_uniformBufferAllocations[i],
			m_uniformBuffers[i]
		);
	}
}

std::shared_ptr<icpMaterialTemplate> icpPrimitiveRendererComponent::AddMaterial(eMaterialShadingModel shading_model)
{
	auto materialSystem = g_system_container.m_renderSystem->GetMaterialSubSystem();
	auto instance = materialSystem->createMaterialInstance(shading_model);
	m_vMaterials.push_back(instance);

	return instance;
}


INCEPTION_END_NAMESPACE