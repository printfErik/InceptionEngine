#pragma once

#include "../core/icpMacros.h"
#include "../core/icpGuid.h"

#include "../scene/icpComponent.h"

INCEPTION_BEGIN_NAMESPACE

enum class ePrimitiveType
{
	CUBE = 0,
	SPHERE = 1,
	NONE = 9999
};

class icpMeshRendererComponent : public icpComponentBase
{
public:
	icpMeshRendererComponent() = default;
	virtual ~icpMeshRendererComponent() = default;

	std::string m_meshResId;
	std::string m_texResId;

	ePrimitiveType m_primitive = ePrimitiveType::NONE;
};


INCEPTION_END_NAMESPACE