#include "icpGLTFLoaderUtil.h"
#include "../core/icpSystemContainer.h"
#include "../core/icpLogSystem.h"
#include "../mesh/icpMeshData.h"

INCEPTION_BEGIN_NAMESPACE

void icpGLTFLoaderUtil::LoadGLTFMesh(tinygltf::Model& gltfModel, std::vector<std::vector<icpMeshData>>& meshDatas)
{
	for (auto meshIdx = 0; meshIdx < gltfModel.meshes.size(); meshIdx++)
	{
		tinygltf::Mesh& gltfMesh = gltfModel.meshes[meshIdx];

		std::vector<icpVertex> vertices;
		std::vector<uint32_t> indices;
		for (auto primitiveIdx = 0; primitiveIdx < gltfMesh.primitives.size(); primitiveIdx++)
		{
			tinygltf::Primitive& gltfPrimitive = gltfMesh.primitives[primitiveIdx];

			icpMeshData meshData;
			meshData.m_eVertexFormat = eVertexFormat::PNCV_F32;

			// Load Index Buffer
			int indexAccessorIdx = gltfPrimitive.indices;

			const tinygltf::Accessor& idxAccessor = gltfModel.accessors[indexAccessorIdx > -1 ? indexAccessorIdx : 0];
			const tinygltf::BufferView& idxBufferView = gltfModel.bufferViews[idxAccessor.bufferView];
			const tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];

			uint32_t indexCount = static_cast<uint32_t>(accessor.count);
			uint8_t* dataPtr = &(buffer.data[accessor.byteOffset + bufferView.byteOffset]);

			std::vector<uint8_t> indexBufferData;
			LoadGLTFBuffer(dataPtr, accessor, bufferView, indexBufferData);
			for (int i = 0; i < indexCount; i++)
			{
				uint32_t index = 0;
				switch (accessor.componentType)
				{
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
				{
					uint32_t* bfr = (uint32_t*)indexBufferData.data();
					index = *(bfr + i);
				}
				break;
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
				{
					uint16_t* bfr = (uint16_t*)indexBufferData.data();
					index = *(bfr + i);
				}
				break;
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
				{
					uint8_t* bfr = (uint8_t*)indexBufferData.data();
					index = *(bfr + i);
				}
				break;
				default:
				{
					ICP_LOG_WARING("Unsupported index format");
				}
				}
				meshData.m_vertexIndices.push_back(index);
			}

			// Load Vertex Data
			assert(gltfPrimitive.attributes.contains("POSITION"));

			int posAccessorIdx = gltfPrimitive.attributes["POSITION"];

			const tinygltf::Accessor& posAccessor = gltfModel.accessors[posAccessorIdx > -1 ? posAccessorIdx : 0];
			const tinygltf::BufferView& posBufferView = gltfModel.bufferViews[posAccessor.bufferView];
			const tinygltf::Buffer& posBuffer = gltfModel.buffers[posBufferView.buffer];

			auto vertexCount = static_cast<uint32_t>(posAccessor.count);
			uint8_t* posDataPtr = &(posBuffer.data[posAccessor.byteOffset + posBufferView.byteOffset]);

			std::vector<uint8_t> posBufferData;
			LoadGLTFBuffer(posDataPtr, posAccessor, posBufferView, posBufferData);

			if (gltfPrimitive.attributes.contains("NORMAL"))
			{
				int normalAccessorIdx = gltfPrimitive.indices;

				const tinygltf::Accessor& accessor = gltfModel.accessors[indexAccessor > -1 ? indexAccessor : 0];
				const tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
				const tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];

				uint32_t indexCount = static_cast<uint32_t>(accessor.count);
				uint8_t* dataPtr = &(buffer.data[accessor.byteOffset + bufferView.byteOffset]);

				std::vector<uint8_t> indexBufferData;
				LoadGLTFBuffer(dataPtr, accessor, bufferView, indexBufferData);
			}
		}
	}
}

void icpGLTFLoaderUtil::LoadGLTFBuffer(
	uint8_t* dataPtr, const tinygltf::Accessor& accessor, const tinygltf::BufferView& bufferView,
	std::vector<uint8_t>& outBuffer)
{
	size_t elementSize = tinygltf::GetComponentSizeInBytes(accessor.componentType);
	size_t elementCount = tinygltf::GetNumComponentsInType(accessor.type);

	elementSize *= elementCount;

	size_t stride = bufferView.byteStride;
	if (stride == 0)
	{
		stride = elementSize;

	}

	size_t indexCount = accessor.count;

	outBuffer.resize(indexCount * elementSize);

	for (int i = 0; i < indexCount; i++) {
		uint8_t* dataindex = dataPtr + stride * i;
		uint8_t* targetptr = outBuffer.data() + elementSize * i;

		memcpy(targetptr, dataindex, elementSize);
	}
}


INCEPTION_END_NAMESPACE

