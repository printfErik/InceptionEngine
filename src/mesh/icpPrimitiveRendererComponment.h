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

class icpPrimitiveRendererComponment : public icpComponentBase
{
public:
	icpPrimitiveRendererComponment() = default;
	virtual ~icpPrimitiveRendererComponment() = default;

	ePrimitiveType m_primitive = ePrimitiveType::NONE;
};


INCEPTION_END_NAMESPACE