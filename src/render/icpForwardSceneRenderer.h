#pragma once
#include "../core/icpMacros.h"
#include <vulkan/vulkan.hpp>

#include "RHI/icpDescirptorSet.h"
#include "RHI/Vulkan/icpVkGPUDevice.h"
#include "icpSceneRenderer.h"

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

class icpForwardSceneRenderer : public std::enable_shared_from_this<icpForwardSceneRenderer>, public icpSceneRenderer
{
public:
	icpForwardSceneRenderer();
	virtual ~icpForwardSceneRenderer() override;

	bool Initialize(std::shared_ptr<icpGPUDevice> vulkanRHI) override;
	void Cleanup() override;
	void Render() override;

	VkCommandBuffer GetMainForwardCommandBuffer(uint32_t curFrame) override;
	VkRenderPass GetMainForwardRenderPass() override;
	VkDescriptorSet GetSceneDescriptorSet(uint32_t curFrame) override;

	icpDescriptorSetLayoutInfo& GetSceneDSLayout() override;

	void CreateSceneCB();
	void UpdateGlobalSceneCB(uint32_t curFrame);

	void CreateGlobalSceneDescriptorSetLayout();
	void AllocateGlobalSceneDescriptorSets();

	void CreateForwardRenderPass();
	void CreateSwapChainFrameBuffers();
	void AllocateCommandBuffers();

	void RecreateSwapChain();
	void CleanupSwapChain();

private:

	void ResetThenBeginCommandBuffer();
	void BeginForwardRenderPass(uint32_t imageIndex);

	void EndForwardRenderPass();
	void EndRecordingCommandBuffer();

	void SubmitCommandList();
	void Present(uint32_t imageIndex);

	std::vector<VmaAllocation> m_vSceneCBAllocations;

	std::vector<VkCommandBuffer> m_vMainForwardCommandBuffers;
	VkRenderPass m_mainForwardRenderPass{ VK_NULL_HANDLE };

	std::vector<VkDescriptorSet> m_vSceneDSs;
	std::vector<VkBuffer> m_vSceneCBs;
	std::vector<VkFramebuffer> m_vSwapChainFrameBuffers;

	icpDescriptorSetLayoutInfo m_sceneDSLayout{};

	uint32_t m_currentFrame = 0;
};

INCEPTION_END_NAMESPACE