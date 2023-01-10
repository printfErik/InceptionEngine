#pragma once

#include "../core/icpMacros.h"

INCEPTION_BEGIN_NAMESPACE

class enum eMaterialModel
{
	LIT_MODEL = 0,
	ONE_TEXTURE_ONLY = 1,
	MATERIAL_MODEL_MAX_ENUM = 9999
};

class icpMaterial
{
public:
	icpMaterial();
	virtual ~icpMaterial();
};

INCEPTION_END_NAMESPACE