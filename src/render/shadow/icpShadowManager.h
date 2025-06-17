#pragma once
#include "../../core/icpMacros.h"

#include "../RHI/icpGPUDevice.h"


INCEPTION_BEGIN_NAMESPACE

// todo
static uint32_t s_csmCascadeCount(4u);
static uint32_t s_cascadeShadowMapResolution(1024u);

class icpShadowManager
{
public:
	icpShadowManager() = default;
	virtual ~icpShadowManager() = default;

	void InitCascadeDistance();
	void UpdateCSMCB(float aspectRatio, const glm::vec3& direction);

private:
	std::vector<float> m_cascadeSplits;
	std::vector<glm::mat4> m_lightProjViews;

};


INCEPTION_END_NAMESPACE