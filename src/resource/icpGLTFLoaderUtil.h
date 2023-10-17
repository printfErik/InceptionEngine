#pragma once
#include <tiny_gltf.h>
#include "../core/icpMacros.h"
#include "../render/icpImageResource.h"

INCEPTION_BEGIN_NAMESPACE
class icpMeshData;

class icpGLTFLoaderUtil
{
public:
	static void LoadGLTFMeshs(tinygltf::Model& gltfModel, std::vector<std::vector<icpMeshData>>& meshDatas);

	static void LoadGLTFBuffer(uint8_t* dataPtr, const tinygltf::Accessor& accessor, const tinygltf::BufferView& bufferView,
		std::vector<uint8_t>& outBuffer);

	static void LoadGLTFIndexBuffer(const tinygltf::Primitive& gltfPrimitive, const tinygltf::Model& gltfModel,
		icpMeshData& meshData);

	static void LoadGLTFVertexBuffer(const tinygltf::Primitive& gltfPrimitive, const tinygltf::Model& gltfModel,
		icpMeshData& meshData);

	static void LoadGLTFTextureSamplers(tinygltf::Model& gltfModel, std::vector<icpSamplerResource>& samplers);

	static void LoadGLTFTextures(tinygltf::Model& gltfModel, const std::vector<icpSamplerResource>& samplers,
		std::vector<icpImageResource>& images);
};

INCEPTION_END_NAMESPACE