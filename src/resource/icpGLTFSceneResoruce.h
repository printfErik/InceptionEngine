#pragma once 
#include "../core/icpMacros.h"
#include "icpResourceBase.h"

INCEPTION_BEGIN_NAMESPACE

class icpGLTFSceneResource : public icpResourceBase
{
public:
	std::unique_ptr<tinygltf::Model> m_gltfModel;
	std::vector<std::vector<icpMeshResource>> m_meshResourceList;
};


INCEPTION_END_NAMESPACE