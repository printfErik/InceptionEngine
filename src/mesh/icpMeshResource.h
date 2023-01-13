#pragma once 
#include "../core/icpMacros.h"
#include "icpMeshData.h"
#include "../resource/icpResourceBase.h"

INCEPTION_BEGIN_NAMESPACE

enum class ePrimitiveType;

class icpMeshResource : public icpResourceBase
{
public:
	icpMeshData m_meshData;

	void prepareRenderResourceForMesh();
};


INCEPTION_END_NAMESPACE