#pragma once
#include "../core/icpMacros.h"
#include <vulkan/vulkan.hpp>

#include "RHI/icpDescirptorSet.h"
#include "RHI/Vulkan/icpVkGPUDevice.h"

INCEPTION_BEGIN_NAMESPACE

enum class eRenderPass
{
	MAIN_FORWARD_PASS = 0,
	UNLIT_PASS,
	EDITOR_UI_PASS,
	RENDER_PASS_COUNT
};

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
	glm::vec3 ambient;
	glm::vec3 diffuse;
	glm::vec3 specular;
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

class icpRenderPassManager : public std::enable_shared_from_this<icpRenderPassManager>
{
public:
	icpRenderPassManager();
	~icpRenderPassManager();

	bool initialize(std::shared_ptr<icpGPUDevice> vulkanRHI);
	void cleanup();
	void render();

	void CreateSceneCB();
	void UpdateGlobalSceneCB(uint32_t curFrame);

	void CreateGlobalSceneDescriptorSetLayout();
	void AllocateGlobalSceneDescriptorSets();

	void CreateForwardRenderPass();
	void CreateSwapChainFrameBuffers();

	void AllocateCommandBuffers();
	void RecreateSwapChain();
	void CleanupSwapChain();

	std::shared_ptr<icpRenderPassBase> accessRenderPass(eRenderPass passType);

	std::vector<VkBuffer> m_vSceneCBs;
	icpDescriptorSetLayoutInfo m_sceneDSLayout{};
	std::vector<VkDescriptorSet> m_vSceneDSs;

	VkRenderPass m_mainForwardRenderPass{ VK_NULL_HANDLE };

	std::vector<VkFramebuffer> m_vSwapChainFrameBuffers;
	std::vector<VkCommandBuffer> m_vMainForwardCommandBuffers;

private:

	std::shared_ptr<icpGPUDevice> m_pDevice = nullptr;

	std::vector<std::shared_ptr<icpRenderPassBase>> m_renderPasses;

	std::vector<VmaAllocation> m_vSceneCBAllocations;
	

	uint32_t m_currentFrame = 0;
};

INCEPTION_END_NAMESPACE