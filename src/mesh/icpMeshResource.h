#pragma once 
#include "../core/icpMacros.h"
#include "icpMeshData.h"
#include "../render/material/icpMaterial.h"
#include "../resource/icpResourceBase.h"

INCEPTION_BEGIN_NAMESPACE

class icpMeshResource : public icpResourceBase
{
public:
	icpMeshData m_meshData;

	std::shared_ptr<icpMaterialInstance> m_pMaterial = nullptr;
};


INCEPTION_END_NAMESPACE