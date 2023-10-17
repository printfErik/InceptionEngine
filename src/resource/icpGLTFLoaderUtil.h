#pragma once
#include "tiny_gltf.h"
#include "../core/icpMacros.h"

INCEPTION_BEGIN_NAMESPACE
class icpMeshData;

class icpGLTFLoaderUtil
{
public:
	static void LoadGLTFMesh(tinygltf::Model& gltfModel, std::vector<std::vector<icpMeshData>>& meshDatas);

	static void LoadGLTFBuffer(uint8_t* dataPtr, const tinygltf::Accessor& accessor, const tinygltf::BufferView& bufferView,
		std::vector<uint8_t>& outBuffer);

};

INCEPTION_END_NAMESPACE