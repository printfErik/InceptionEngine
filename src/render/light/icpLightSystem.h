#pragma once

#include "../../core/icpMacros.h"

INCEPTION_BEGIN_NAMESPACE
struct perFrameCB;

static constexpr uint32_t MAX_POINT_LIGHT_NUMBER = 8;

class icpLightSystem
{
public:
	icpLightSystem();
	virtual ~icpLightSystem();

	void UpdateLightCB(perFrameCB& cb);
private:

};

INCEPTION_END_NAMESPACE