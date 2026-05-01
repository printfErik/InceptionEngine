#pragma once
#include "../core/icpMacros.h"
#include "RHI/icpGPUBuffer.h"
#include "RHI/icpGPUDevice.h"
#include "light/icpLightSystem.h"

#include <glm/glm.hpp>

INCEPTION_BEGIN_NAMESPACE

struct DirectionalLightRenderResource
{
	glm::vec4 direction;
	glm::vec4 color;
};

struct PointLightRenderResource
{
	glm::mat4 viewMatrices[6];
	glm::vec4 color;
	glm::vec3 position;
	float _padding = 0.f;
};

struct SpotLightRenderResource
{
	glm::mat4 viewMatrices;
	glm::vec4 color;
	glm::vec3 position;
	float innerConeAngle;
	glm::vec3 direction;
	float outerConeAngle;
};

struct perFrameCB
{
	glm::mat4 view;
	glm::mat4 projection;
	glm::mat4 invViewProjection;
	glm::vec3 camPos;
	float pointLightNumber = 0.f;
	DirectionalLightRenderResource dirLight;
	PointLightRenderResource pointLight[MAX_POINT_LIGHT_NUMBER];
};

enum class eRenderPass
{
	CSM_PASS = 0,
	GBUFFER_PASS,
	GTAP_PASS,
	DEFERRED_COMPOSITION_PASS,
	TRANSLUCENT_PASS,
	EDITOR_UI_PASS,
	RENDER_PASS_COUNT
};

class icpSceneRenderer
{
public:
	icpSceneRenderer() = default;
	virtual ~icpSceneRenderer() = default;

	virtual bool Initialize(std::shared_ptr<icpGPUDevice> rhi) = 0;
	virtual void Cleanup() {}
	virtual void Render() = 0;

	uint32_t GetCurrentFrame() const { return m_currentFrame; }

	std::vector<icpBufferRenderResource> SceneUBOs;

protected:
	std::shared_ptr<icpGPUDevice> m_pDevice = nullptr;
	uint32_t m_currentFrame = 0;
};

INCEPTION_END_NAMESPACE
