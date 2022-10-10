#pragma once

#include "../core/icpMacros.h"
#include "../core/icpGuid.h"
#include "icpComponent.h"

INCEPTION_BEGIN_NAMESPACE

class icpEntityDataComponent : public icpComponentBase
{
public:
	icpEntityDataComponent() = default;
	virtual ~icpEntityDataComponent() = default;



	icpGuid m_guid;
	
	std::string m_name;

};


INCEPTION_END_NAMESPACE