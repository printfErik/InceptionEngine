#pragma once
#include "../core/icpMacros.h"
#include "light/icpLightSystem.h"
#include "RHI/icpDescirptorSet.h"
#include "RHI/icpGPUDevice.h"


INCEPTION_BEGIN_NAMESPACE

class icpRenderPassBase;


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
	glm::vec3 camPos;
	float pointLightNumber = 0.f;
	DirectionalLightRenderResource dirLight;
	PointLightRenderResource pointLight[MAX_POINT_LIGHT_NUMBER];
};


enum class eRenderPass
{
	MAIN_FORWARD_PASS = 0,
	UNLIT_PASS,
	CSM_PASS,
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
	void UpdateCSMCB(uint32_t curFrame);

	void CreateGlobalSceneDescriptorSetLayout();
	void AllocateGlobalSceneDescriptorSets();

	VkDescriptorSet GetSceneDescriptorSet(uint32_t curFrame);
	icpDescriptorSetLayoutInfo& GetSceneDSLayout();

	uint32_t GetCurrentFrame() const;

protected:
	std::shared_ptr<icpGPUDevice> m_pDevice = nullptr;
	std::map<eRenderPass, std::shared_ptr<icpRenderPassBase>> m_renderPasses;

	std::vector<VkBuffer> m_vSceneCBs;
	std::vector<VmaAllocation> m_vSceneCBAllocations;

	icpDescriptorSetLayoutInfo m_sceneDSLayout{};
	std::vector<VkDescriptorSet> m_vSceneDSs;

	uint32_t m_currentFrame = 0;
private:
};

inline icpSceneRenderer::~icpSceneRenderer() = default;

INCEPTION_END_NAMESPACE