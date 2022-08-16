#pragma once
#include "icpMacros.h"

INCEPTION_BEGIN_NAMESPACE

class icpRHIBase
{
public:
	icpRHIBase() = default;

	virtual	~icpRHIBase() = 0;

	virtual bool initialize() = 0;

};

inline icpRHIBase::~icpRHIBase() = default;

INCEPTION_END_NAMESPACE