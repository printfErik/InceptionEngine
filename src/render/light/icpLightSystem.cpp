#include "icpLightSystem.h"
#include "../../core/icpSystemContainer.h"
#include "icpLightComponent.h"
#include "../icpSceneRenderer.h"

INCEPTION_BEGIN_NAMESPACE

icpLightSystem::icpLightSystem()
{
	
}

icpLightSystem::~icpLightSystem()
{
	
}

void icpLightSystem::UpdateLightCB(perFrameCB& CBPerFrame)
{
	auto directionalLightView = g_system_container.m_sceneSystem->m_registry.view<icpDirectionalLightComponent>();

	for (auto& light : directionalLightView)
	{
		auto& lightComp = directionalLightView.get<icpDirectionalLightComponent>(light);
		CBPerFrame.dirLight.color = glm::vec4(lightComp.m_color, 1.f);
		CBPerFrame.dirLight.direction = glm::vec4(lightComp.m_direction, 0.f);
	}

	auto pointLightView = g_system_container.m_sceneSystem->m_registry.view<icpPointLightComponent>();

	uint32_t pointLightIdx = 0;
	for (auto& point : pointLightView)
	{
		auto& lightComp = pointLightView.get<icpPointLightComponent>(point);
		CBPerFrame.pointLight[pointLightIdx].color = glm::vec4(lightComp.m_color, 1.f);
		CBPerFrame.pointLight[pointLightIdx].position = lightComp.m_position;

		GeneratePointViewMatrices(CBPerFrame.pointLight[pointLightIdx], lightComp);

		pointLightIdx++;
	}

	CBPerFrame.pointLightNumber = pointLightView.empty() ? 0.f : (float)pointLightIdx + 1.f;

}

void icpLightSystem::GeneratePointViewMatrices(PointLightRenderResource& pointLight, const icpPointLightComponent& icpComp)
{
	pointLight.viewMatrices[0] = glm::lookAt(icpComp.m_position, glm::vec3(1.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
	pointLight.viewMatrices[1] = glm::lookAt(icpComp.m_position, glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
	pointLight.viewMatrices[2] = glm::lookAt(icpComp.m_position, glm::vec3(0.0, 1.0, 0.0), glm::vec3(1.0, .0, 0.0));
	pointLight.viewMatrices[3] = glm::lookAt(icpComp.m_position, glm::vec3(0.0, -1.0, 0.0), glm::vec3(1.0, 0.0, 0.0));
	pointLight.viewMatrices[4] = glm::lookAt(icpComp.m_position, glm::vec3(0.0, 0.0, 1.0), glm::vec3(0.0, 1.0, 0.0));
	pointLight.viewMatrices[5] = glm::lookAt(icpComp.m_position, glm::vec3(0.0, 0.0, -1.0), glm::vec3(0.0, 1.0, 0.0));
}


INCEPTION_END_NAMESPACE