#include "icpGLTFLoaderUtil.h"

#include "icpSamplerResource.h"
#include "../core/icpSystemContainer.h"
#include "../core/icpLogSystem.h"
#include "../mesh/icpMeshData.h"

INCEPTION_BEGIN_NAMESPACE

void icpGLTFLoaderUtil::LoadGLTFMeshs(tinygltf::Model& gltfModel, std::vector<std::vector<icpMeshData>>& meshDatas)
{
	meshDatas.resize(gltfModel.meshes.size());
	for (auto meshIdx = 0; meshIdx < gltfModel.meshes.size(); meshIdx++)
	{
		tinygltf::Mesh& gltfMesh = gltfModel.meshes[meshIdx];

		std::vector<icpVertex> vertices;
		std::vector<uint32_t> indices;

		std::vector<icpMeshData> primitives(gltfMesh.primitives.size());

		for (auto primitiveIdx = 0; primitiveIdx < gltfMesh.primitives.size(); primitiveIdx++)
		{
			tinygltf::Primitive& gltfPrimitive = gltfMesh.primitives[primitiveIdx];

			icpMeshData meshData;
			meshData.m_eVertexFormat = eVertexFormat::PNCV_F32;

			LoadGLTFIndexBuffer(gltfPrimitive, gltfModel, meshData);
			LoadGLTFVertexBuffer(gltfPrimitive, gltfModel, meshData);

			primitives[primitiveIdx] = meshData;
		}

		meshDatas[meshIdx] = primitives;
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

void icpGLTFLoaderUtil::LoadGLTFIndexBuffer(
	const tinygltf::Primitive& gltfPrimitive, const tinygltf::Model& gltfModel,
	icpMeshData& meshData)
{
	// Load Index Buffer
	int accessorIdx = gltfPrimitive.indices;

	const tinygltf::Accessor& accessor = gltfModel.accessors[accessorIdx > -1 ? accessorIdx : 0];
	const tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
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
}

void icpGLTFLoaderUtil::LoadGLTFVertexBuffer(
	const tinygltf::Primitive& gltfPrimitive, const tinygltf::Model& gltfModel,
	icpMeshData& meshData)
{
	// Load Vertex Data
	assert(gltfPrimitive.attributes.contains("POSITION"));

	int posAccessorIdx = gltfPrimitive.attributes["POSITION"];

	const tinygltf::Accessor& posAccessor = gltfModel.accessors[posAccessorIdx > -1 ? posAccessorIdx : 0];
	assert(posAccessor.type == TINYGLTF_TYPE_VEC3);
	assert(posAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

	const tinygltf::BufferView& posBufferView = gltfModel.bufferViews[posAccessor.bufferView];
	const tinygltf::Buffer& posBuffer = gltfModel.buffers[posBufferView.buffer];

	auto vertexCount = static_cast<uint32_t>(posAccessor.count);
	uint8_t* posDataPtr = &(posBuffer.data[posAccessor.byteOffset + posBufferView.byteOffset]);

	std::vector<uint8_t> posBufferData;
	LoadGLTFBuffer(posDataPtr, posAccessor, posBufferView, posBufferData);

	std::vector<uint8_t> normalBufferData;
	int normalAccessorIdx = -1;
	if (gltfPrimitive.attributes.contains("NORMAL"))
	{
		normalAccessorIdx = gltfPrimitive.attributes["NORMAL"];
		const tinygltf::Accessor& accessor = gltfModel.accessors[normalAccessorIdx > -1 ? normalAccessorIdx : 0];
		assert(accessor.type == TINYGLTF_TYPE_VEC3);
		assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

		const tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
		const tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];

		uint32_t indexCount = static_cast<uint32_t>(accessor.count);
		uint8_t* dataPtr = &(buffer.data[accessor.byteOffset + bufferView.byteOffset]);

		LoadGLTFBuffer(dataPtr, accessor, bufferView, normalBufferData);
	}

	std::vector<uint8_t> colorBufferData;
	int colorAccessorIdx = -1;
	if (gltfPrimitive.attributes.contains("COLOR0"))
	{
		colorAccessorIdx = gltfPrimitive.attributes["COLOR0"];
		const tinygltf::Accessor& accessor = gltfModel.accessors[colorAccessorIdx > -1 ? colorAccessorIdx : 0];
		assert(accessor.type == TINYGLTF_TYPE_VEC3);
		assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

		const tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
		const tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];

		uint32_t indexCount = static_cast<uint32_t>(accessor.count);
		uint8_t* dataPtr = &(buffer.data[accessor.byteOffset + bufferView.byteOffset]);

		LoadGLTFBuffer(dataPtr, accessor, bufferView, colorBufferData);
	}

	std::vector<uint8_t> texCoordBufferData;
	int tcAccessorIdx = -1;
	if (gltfPrimitive.attributes.contains("TEXCOORD0"))
	{
		tcAccessorIdx = gltfPrimitive.attributes["TEXCOORD0"];
		const tinygltf::Accessor& accessor = gltfModel.accessors[tcAccessorIdx > -1 ? tcAccessorIdx : 0];
		assert(accessor.type == TINYGLTF_TYPE_VEC2);
		assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

		const tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
		const tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];

		uint32_t indexCount = static_cast<uint32_t>(accessor.count);
		uint8_t* dataPtr = &(buffer.data[accessor.byteOffset + bufferView.byteOffset]);

		LoadGLTFBuffer(dataPtr, accessor, bufferView, texCoordBufferData);
	}

	for (size_t i = 0; i < vertexCount; i++)
	{
		icpVertex vertex;

		float* pPos = (float*)posBufferData.data();

		vertex.position[0] = *(pPos + (i * 3) + 0);
		vertex.position[1] = *(pPos + (i * 3) + 1);
		vertex.position[2] = *(pPos + (i * 3) + 2);

		float* pNormal = (float*)normalBufferData.data();

		vertex.normal[0] = *(pNormal + (i * 3) + 0);
		vertex.normal[1] = *(pNormal + (i * 3) + 1);
		vertex.normal[2] = *(pNormal + (i * 3) + 2);

		float* pColor = (float*)normalBufferData.data();

		vertex.normal[0] = *(pColor + (i * 3) + 0);
		vertex.normal[1] = *(pColor + (i * 3) + 1);
		vertex.normal[2] = *(pColor + (i * 3) + 2);

		float* pTc = (float*)texCoordBufferData.data();

		vertex.texCoord[0] = *(pTc + (i * 2) + 0);
		vertex.texCoord[1] = *(pTc + (i * 2) + 1);

		meshData.m_vertices.push_back(vertex);
	}
}

VkSamplerAddressMode getVkWrapMode(int32_t wrapMode)
{
	switch (wrapMode)
	{
	case -1:
	case 10497:
		return VK_SAMPLER_ADDRESS_MODE_REPEAT;
	case 33071:
		return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	case 33648:
		return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	}

	ICP_LOG_ERROR("Unknown wrap mode for getVkWrapMode: ", wrapMode);
	return VK_SAMPLER_ADDRESS_MODE_REPEAT;
}

VkFilter getVkFilterMode(int32_t filterMode)
{
	switch (filterMode)
	{
	case -1:
	case 9728:
		return VK_FILTER_NEAREST;
	case 9729:
		return VK_FILTER_LINEAR;
	case 9984:
		return VK_FILTER_NEAREST;
	case 9985:
		return VK_FILTER_NEAREST;
	case 9986:
		return VK_FILTER_LINEAR;
	case 9987:
		return VK_FILTER_LINEAR;
	}

	ICP_LOG_ERROR("Unknown filter mode for getVkFilterMode: ", filterMode);
	return VK_FILTER_NEAREST;
}

void icpGLTFLoaderUtil::LoadGLTFTextureSamplers(tinygltf::Model& gltfModel, std::vector<icpSamplerResource>& samplers)
{
	for (auto& sampler : gltfModel.samplers)
	{
		icpSamplerResource textureSampler;

		textureSampler.minFilter = getVkFilterMode(sampler.minFilter);
		textureSampler.magFilter = getVkFilterMode(sampler.magFilter);
		textureSampler.addressModeU = getVkWrapMode(sampler.wrapS);
		textureSampler.addressModeV = getVkWrapMode(sampler.wrapT);
		textureSampler.addressModeW = textureSampler.addressModeV;

		samplers.push_back(textureSampler);
	}
}

void icpGLTFLoaderUtil::LoadGLTFTextures(tinygltf::Model& gltfModel, const std::vector<icpSamplerResource>& samplers,
	std::vector<icpImageResource>& images)
{
	for (tinygltf::Texture& tex : gltfModel.textures) 
	{
		tinygltf::Image image = gltfModel.images[tex.source];
		icpSamplerResource textureSampler;
		if (tex.sampler == -1) 
		{
			// No sampler specified, use a default one
			textureSampler.magFilter = VK_FILTER_LINEAR;
			textureSampler.minFilter = VK_FILTER_LINEAR;
			textureSampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			textureSampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			textureSampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		}
		else 
		{
			textureSampler = samplers[tex.sampler];
		}

		icpImageResource imageRes;
		// todo
		
		images.push_back(imageRes);
	}
}


INCEPTION_END_NAMESPACE
