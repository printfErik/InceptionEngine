#include "icpShadowManager.h"

#include "../icpCameraSystem.h"
#include "../../core/icpSystemContainer.h"

INCEPTION_BEGIN_NAMESPACE
	void icpShadowManager::UpdateCSMCB()
{
	// Split View Frustum
    std::vector<float> cascadeSplits(s_csmCascadeCount + 1);

    float nearPlane = g_system_container.m_cameraSystem->getCurrentCamera()->m_near;
    float farPlane = g_system_container.m_cameraSystem->getCurrentCamera()->m_far;

    cascadeSplits[0] = nearPlane;
    cascadeSplits[s_csmCascadeCount] = farPlane;
    for (int i = 1; i < s_csmCascadeCount; ++i) {
        float si = (float)i / s_csmCascadeCount;
        float log = nearPlane * std::pow(farPlane / nearPlane, si);
        float lin = nearPlane + (farPlane - nearPlane) * si;
        float d = 0.5f * (log - lin) + lin;
        cascadeSplits[i] = d;
    }
}


INCEPTION_END_NAMESPACE
