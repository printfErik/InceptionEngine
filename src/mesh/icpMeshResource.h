#pragma once 
#include "../core/icpMacros.h"
#include "icpMeshData.h"
#include "../resource/icpResourceBase.h"

INCEPTION_BEGIN_NAMESPACE

class icpMeshResource : public icpResourceBase
{
public:
	icpMeshData m_meshData;
};


INCEPTION_END_NAMESPACE