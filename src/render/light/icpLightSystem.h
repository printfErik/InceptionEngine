#pragma once

#include "../../core/icpMacros.h"

INCEPTION_BEGIN_NAMESPACE
struct perFrameCB;

class icpLightSystem
{
public:
	icpLightSystem();
	virtual ~icpLightSystem();

	void UpdateLightCB(perFrameCB& cb);
private:

};

INCEPTION_END_NAMESPACE