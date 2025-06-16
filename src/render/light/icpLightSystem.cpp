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

	/*
	auto spotLightView = g_system_container.m_sceneSystem->m_registry.view<icpSpotLightComponent>();

	
	uint32_t spotLightIdx = 0;
	for (auto& spot : spotLightView)
	{
		auto& lightComp = spotLightView.get<icpSpotLightComponent>(spot);
		CBPerFrame.spotLight[spotLightIdx].color = glm::vec4(lightComp.m_color, 1.f);
		CBPerFrame.spotLight[spotLightIdx].position = lightComp.m_position;
		CBPerFrame.spotLight[spotLightIdx].direction = lightComp.m_direction;

		CBPerFrame.spotLight[spotLightIdx].innerConeAngle = lightComp.m_innerConeAngle;
		CBPerFrame.spotLight[spotLightIdx].outerConeAngle = lightComp.m_outerConeAngle;

		CBPerFrame.spotLight[spotLightIdx].viewMatrices = glm::lookAt(lightComp.m_position, lightComp.m_position + lightComp.m_direction, glm::vec3(0.0, 1.0, 0.0));

		spotLightIdx++;
	}

	CBPerFrame.spotLightNumber = spotLightView.empty() ? 0.f : (float)spotLightIdx + 1.f;
	*/
}

void icpLightSystem::GeneratePointViewMatrices(PointLightRenderResource& pointLight, const icpPointLightComponent& icpComp)
{
	pointLight.viewMatrices[0] = glm::lookAt(icpComp.m_position, icpComp.m_position + glm::vec3(1.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
	pointLight.viewMatrices[1] = glm::lookAt(icpComp.m_position, icpComp.m_position + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
	pointLight.viewMatrices[2] = glm::lookAt(icpComp.m_position, icpComp.m_position + glm::vec3(0.0, 1.0, 0.0), glm::vec3(1.0, .0, 0.0));
	pointLight.viewMatrices[3] = glm::lookAt(icpComp.m_position, icpComp.m_position + glm::vec3(0.0, -1.0, 0.0), glm::vec3(1.0, 0.0, 0.0));
	pointLight.viewMatrices[4] = glm::lookAt(icpComp.m_position, icpComp.m_position + glm::vec3(0.0, 0.0, 1.0), glm::vec3(0.0, 1.0, 0.0));
	pointLight.viewMatrices[5] = glm::lookAt(icpComp.m_position, icpComp.m_position + glm::vec3(0.0, 0.0, -1.0), glm::vec3(0.0, 1.0, 0.0));
}

void icpLightSystem::GenerateSpotViewMatrices(SpotLightRenderResource& SpotLight, const icpSpotLightComponent& icpSpotLightComp)
{
	
}



INCEPTION_END_NAMESPACE