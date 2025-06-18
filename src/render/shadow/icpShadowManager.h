#pragma once
#include "../../core/icpMacros.h"

#include "../RHI/icpGPUDevice.h"
#include "../RHI/icpDescirptorSet.h"


INCEPTION_BEGIN_NAMESPACE

// todo
static constexpr uint32_t s_csmCascadeCount(4u);
static constexpr uint32_t s_cascadeShadowMapResolution(1024u);

struct FCascadeSMCB
{
	glm::mat4 lightProjView[s_csmCascadeCount];
	float cascadeSplit[s_csmCascadeCount];
};

class icpShadowManager
{
public:
	icpShadowManager() = default;
	virtual ~icpShadowManager() = default;

	void InitCascadeDistance();
	void UpdateCSMCB(float aspectRatio, const glm::vec3& direction, uint32_t curFrame);

	void CreateCSMCB();
	void CreateCSMDSLayout();
	void AllocateCSMDS();

	std::vector<VkBuffer> m_csmCBs;
	std::vector<VmaAllocation> m_csmCBAllocations;

	icpDescriptorSetLayoutInfo m_csmDSLayout{};
	std::vector<VkDescriptorSet> m_csmDSs;

	std::vector<float> m_cascadeSplits;
	std::vector<glm::mat4> m_lightProjViews;
	std::shared_ptr<icpGPUDevice> m_pDevice = nullptr;
private:
	

	
};


INCEPTION_END_NAMESPACE