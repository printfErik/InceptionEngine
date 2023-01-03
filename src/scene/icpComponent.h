#pragma once

#include "../core/icpMacros.h"
#include "icpEntity.h"


INCEPTION_BEGIN_NAMESPACE

class icpComponentBase
{
public:
	icpComponentBase();
	virtual ~icpComponentBase();

	icpGameEntity* m_possessor;

private:



};


INCEPTION_END_NAMESPACE