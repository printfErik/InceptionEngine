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

	void UpdateCSMCB(uint32_t cascadeIndex, uint32_t curFrame);
	void UpdateCascadeShadowMapCB(uint32_t curFrame);

	void CreateCSMCB();
	void CreateCSMDSLayout();
	void AllocateCSMDS();

	std::vector<VkBuffer> m_csmCBs;
	std::vector<VmaAllocation> m_csmCBAllocations;

	icpDescriptorSetLayoutInfo m_csmDSLayout{};
	std::vector<VkDescriptorSet> m_csmDSs;

	std::vector<VkBuffer> m_cascadeShadowMapCBs;
	std::vector<VmaAllocation> m_cascadeShadowMapCBAllocations;

	icpDescriptorSetLayoutInfo m_cascadeShadowMapDSLayout{};
	std::vector<VkDescriptorSet> m_cascadeShadowMapDSs;
	
	std::vector<float> m_cascadeSplits;
	std::vector<glm::mat4> m_lightProjViews;
	std::shared_ptr<icpGPUDevice> m_pDevice = nullptr;
private:
	

	
};


INCEPTION_END_NAMESPACE