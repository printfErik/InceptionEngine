#pragma once

#include "../../core/icpMacros.h"

INCEPTION_BEGIN_NAMESPACE
	class icpPointLightComponent;
	struct PointLightRenderResource;
	struct perFrameCB;

static constexpr uint32_t MAX_POINT_LIGHT_NUMBER = 4;

static constexpr uint32_t MAX_SPOT_LIGHT_NUMBER = 4;


class icpLightSystem
{
public:
	icpLightSystem();
	virtual ~icpLightSystem();

	void UpdateLightCB(perFrameCB& cb);

	void GeneratePointViewMatrices(PointLightRenderResource& pointLight, const icpPointLightComponent& icpComp);
	void GenerateSpotViewMatrices()
private:

};

INCEPTION_END_NAMESPACE