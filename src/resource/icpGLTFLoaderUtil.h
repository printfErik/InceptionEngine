#pragma once

#include "../core/icpMacros.h"
#include "../render/icpImageResource.h"
#include "../mesh/icpMeshResource.h"

#include <tiny_gltf.h>

INCEPTION_BEGIN_NAMESPACE

class icpGuid;

class icpMeshData;

class icpGLTFLoaderUtil
{
public:
	static void LoadGLTFMeshs(tinygltf::Model& gltf, std::vector<std::shared_ptr<icpMaterialInstance>>& materials, 
		std::vector<std::vector<icpMeshResource>>& meshResources);

	static void LoadGLTFBuffer(uint8_t* dataPtr, const tinygltf::Accessor& accessor, const tinygltf::BufferView& bufferView,
		std::vector<uint8_t>& outBuffer);

	static void LoadGLTFIndexBuffer(const tinygltf::Primitive& gltfPrimitive, const tinygltf::Model& gltfModel,
		icpMeshData& meshData);

	static void LoadGLTFVertexBuffer(const tinygltf::Primitive& gltfPrimitive, const tinygltf::Model& gltfModel,
		icpMeshData& meshData);

	static void LoadGLTFTextureSamplers(tinygltf::Model& gltfModel, std::vector<icpSamplerResource>& samplers);

	static void LoadGLTFTextures(tinygltf::Model& gltfModel, const std::vector<icpSamplerResource>& samplers,
		std::vector<icpImageResource>& images);

	static void LoadGLTFMaterials(tinygltf::Model& gltfModel, std::vector<icpImageResource>& images, 
		std::vector<std::shared_ptr<icpMaterialInstance>>& materials);

	static void LoadGLTFScene(tinygltf::Model& gltfModel, std::vector<std::vector<icpMeshResource>>& meshResources);

	static void LoadGLTFNode(tinygltf::Model& gltfModel, int nodeIdx, icpGuid parentGuid, std::vector<std::vector<icpMeshResource>>& meshResources);
};

INCEPTION_END_NAMESPACE