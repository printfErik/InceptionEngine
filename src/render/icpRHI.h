#pragma once
#include "../core/icpMacros.h"
#include "icpWindowSystem.h"

INCEPTION_BEGIN_NAMESPACE
	class icpRHIBase
{
public:
	icpRHIBase() = default;

	virtual	~icpRHIBase() = 0;

	virtual bool initialize(std::shared_ptr<icpWindowSystem> window_system) = 0;

};

inline icpRHIBase::~icpRHIBase() = default;

INCEPTION_END_NAMESPACE