#pragma once

#include "../../core/icpMacros.h"

INCEPTION_BEGIN_NAMESPACE

enum class eMaterialModel
{
	DIFFUSE_SPECULAR = 0,
	ONE_TEXTURE_ONLY = 1,
	PBR = 2,
	MATERIAL_MODEL_MAX_ENUM = 9999
};

class icpSimpleMaterial
{
public:
	icpSimpleMaterial() = default;
	virtual ~icpSimpleMaterial() = default;

	std::string m_diffuseMapTexResId;
	std::string m_specularMapTexResId;
};

INCEPTION_END_NAMESPACE