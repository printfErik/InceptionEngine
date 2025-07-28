#pragma once
#include "../../core/icpMacros.h"

#include "../RHI/icpGPUDevice.h"
#include "../RHI/icpDescirptorSet.h"


INCEPTION_BEGIN_NAMESPACE

// todo
static constexpr uint32_t s_csmCascadeCount(4u);
static constexpr uint32_t s_cascadeShadowMapResolution(1024u);


struct UBOCSM
{
	glm::vec4 CSMSplits;
	glm::mat4 CSMLightProjViewMat[4];
};

class icpShadowManager
{
public:
	icpShadowManager() = default;
	virtual ~icpShadowManager() = default;

	void InitCascadeDistance();
	void UpdateCSMProjViewMat(float aspectRatio, const glm::vec3& direction, uint32_t curFrame);
	void UpdateCascadeShadowMapCB(uint32_t curFrame);
	void CreateCSMCB();

	std::vector<icpBufferRenderResource> CSMUBOs;
	
	std::vector<float> m_cascadeSplits;
	std::vector<glm::mat4> m_lightProjViews;
	std::shared_ptr<icpGPUDevice> m_pDevice = nullptr;
private:
	

};


INCEPTION_END_NAMESPACE