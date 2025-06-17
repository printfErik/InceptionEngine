#include "icpShadowManager.h"
#include <glm/gtc/matrix_transform.hpp>
#include "../icpCameraSystem.h"
#include "../../core/icpSystemContainer.h"

INCEPTION_BEGIN_NAMESPACE

void icpShadowManager::InitCascadeDistance()
{
    // Split View Frustum
    m_cascadeSplits.resize(s_csmCascadeCount + 1);

    float nearPlane = g_system_container.m_cameraSystem->getCurrentCamera()->m_near;
    float farPlane = g_system_container.m_cameraSystem->getCurrentCamera()->m_far;

    m_cascadeSplits[0] = nearPlane;
    m_cascadeSplits[s_csmCascadeCount] = farPlane;
    for (int i = 1; i < s_csmCascadeCount; ++i) 
    {
        float si = (float)i / s_csmCascadeCount;
        float log = nearPlane * std::pow(farPlane / nearPlane, si);
        float lin = nearPlane + (farPlane - nearPlane) * si;
        float d = 0.5f * (log - lin) + lin;
        m_cascadeSplits[i] = d;
    }

    m_lightProjViews.resize(s_csmCascadeCount);
}

glm::vec3 computeAABBMin(const std::vector<glm::vec3>& pts) {
    glm::vec3 mn = pts[0];
    for (size_t i = 1; i < pts.size(); ++i) {
        mn = glm::min(mn, pts[i]);
    }
    return mn;
}
glm::vec3 computeAABBMax(const std::vector<glm::vec3>& pts) {
    glm::vec3 mx = pts[0];
    for (size_t i = 1; i < pts.size(); ++i) {
        mx = glm::max(mx, pts[i]);
    }
    return mx;
}

void icpShadowManager::UpdateCSMCB(float aspectRatio, const glm::vec3& direction)
{
    auto camera = g_system_container.m_cameraSystem->getCurrentCamera();
    auto viewMat = camera->m_viewMatrix;

    auto invViewMat = glm::inverse(viewMat);

	// Camera space 8 points
    std::vector<std::vector<glm::vec3>> cascadePointsCS;
	for (uint32_t i = 0; i < s_csmCascadeCount; i++)
	{
        auto near = 0.f - m_cascadeSplits[i];
        auto far = 0.f - m_cascadeSplits[i + 1];
        auto halfHeight = glm::tan(camera->m_fov / 2.f);
        auto halfWidth = halfHeight * aspectRatio;


        // To world space
        std::vector<glm::vec3> pointsCS{
            invViewMat * glm::vec4(halfWidth, halfHeight, near, 1.f),
            invViewMat * glm::vec4(-halfWidth, halfHeight, near, 1.f),
            invViewMat * glm::vec4(-halfWidth, -halfHeight, near, 1.f),
            invViewMat * glm::vec4(halfWidth,  -halfHeight, near, 1.f),
            invViewMat * glm::vec4(halfWidth, halfHeight, far, 1.f),
        	invViewMat * glm::vec4(-halfWidth, halfHeight, far, 1.f),
        	invViewMat * glm::vec4(-halfWidth, -halfHeight, far, 1.f),
        	invViewMat * glm::vec4(halfWidth, -halfHeight, far, 1.f)
        };

        cascadePointsCS.push_back(pointsCS);
	}


    for (uint32_t i = 0; i < s_csmCascadeCount; i++)
    {
        glm::vec3 aabbMin = computeAABBMin(cascadePointsCS[i]);
        glm::vec3 aabbMax = computeAABBMax(cascadePointsCS[i]);

        glm::vec3 center = (aabbMin + aabbMax) / 2.f;

        glm::vec3 lightSpaceCamera = center - direction * 1000.f; // todo: large value;

        glm::mat4 viewMatrix = glm::lookAt(lightSpaceCamera, direction, glm::vec3(0.f, 0.f, 1.f));

        glm::vec3 aabbminLS = viewMatrix * glm::vec4(aabbMin, 1.f);
        glm::vec3 aabbmaxLS = viewMatrix * glm::vec4(aabbMax, 1.f);

        glm::mat4 projMatrix = glm::ortho(aabbminLS.x, aabbmaxLS.x, aabbminLS.y, aabbmaxLS.y,
            -aabbmaxLS.z, - aabbminLS.z);

        m_lightProjViews[i] = projMatrix * viewMatrix;
    }


}

INCEPTION_END_NAMESPACE
