#pragma once

#include "../core/icpMacros.h"
#include "../core/icpGuid.h"

#include "../scene/icpComponent.h"

INCEPTION_BEGIN_NAMESPACE


class icpMeshRendererComponent : public icpComponentBase
{
public:
	icpMeshRendererComponent() = default;
	virtual ~icpMeshRendererComponent() = default;

	std::string m_meshResId;
	std::string m_texResId;
};


INCEPTION_END_NAMESPACE