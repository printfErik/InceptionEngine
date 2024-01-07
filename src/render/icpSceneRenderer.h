#pragma once
#include "../core/icpMacros.h"
#include "RHI/icpDescirptorSet.h"
#include "RHI/icpGPUDevice.h"


INCEPTION_BEGIN_NAMESPACE

class icpRenderPassBase;

#define MAX_POINT_LIGHT_COUNT 4

struct DirectionalLightRenderResource
{
	glm::vec4 direction;
	glm::vec4 color;
};

struct PointLightRenderResource
{
	glm::vec3 position;
	glm::vec4 color;
	float constant = 0.f;
	float linear = 0.f;
	float quadratic = 0.f;
};

struct perFrameCB
{
	glm::mat4 view;
	glm::mat4 projection;
	glm::vec3 camPos;
	float pointLightNumber = 0.f;
	DirectionalLightRenderResource dirLight;
	PointLightRenderResource pointLight[MAX_POINT_LIGHT_COUNT];
};


enum class eRenderPass
{
	MAIN_FORWARD_PASS = 0,
	UNLIT_PASS,
	GBUFFER_PASS,
	DEFERRED_COMPOSITION_PASS,
	EDITOR_UI_PASS,
	RENDER_PASS_COUNT
};

class icpSceneRenderer
{
public:
	icpSceneRenderer() = default;
	virtual ~icpSceneRenderer() = 0;

	virtual bool Initialize(std::shared_ptr<icpGPUDevice> vulkanRHI) = 0;
	virtual void Cleanup();
	virtual void Render() = 0;

	// Forward
	virtual VkCommandBuffer GetMainForwardCommandBuffer(uint32_t curFrame) = 0;
	virtual VkRenderPass GetMainForwardRenderPass() = 0;

	// Deferred
	virtual VkRenderPass GetGBufferRenderPass() = 0;
	virtual VkCommandBuffer GetDeferredCommandBuffer(uint32_t curFrame) = 0;
	virtual VkImageView GetGBufferAView() = 0;
	virtual VkImageView GetGBufferBView() = 0;
	virtual VkImageView GetGBufferCView() = 0;

	std::shared_ptr<icpRenderPassBase> AccessRenderPass(eRenderPass passType);

	void CreateSceneCB();
	void UpdateGlobalSceneCB(uint32_t curFrame);
	void CreateGlobalSceneDescriptorSetLayout();
	void AllocateGlobalSceneDescriptorSets();

	VkDescriptorSet GetSceneDescriptorSet(uint32_t curFrame);
	icpDescriptorSetLayoutInfo& GetSceneDSLayout();

protected:
	std::shared_ptr<icpGPUDevice> m_pDevice = nullptr;
	std::vector<std::shared_ptr<icpRenderPassBase>> m_renderPasses;

	std::vector<VkBuffer> m_vSceneCBs;
	std::vector<VmaAllocation> m_vSceneCBAllocations;

	icpDescriptorSetLayoutInfo m_sceneDSLayout{};
	std::vector<VkDescriptorSet> m_vSceneDSs;

	

	uint32_t m_currentFrame = 0;
private:
};

inline icpSceneRenderer::~icpSceneRenderer() = default;

INCEPTION_END_NAMESPACE