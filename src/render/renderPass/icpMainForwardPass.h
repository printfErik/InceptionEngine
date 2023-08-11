#pragma once
#include "../../core/icpMacros.h"
#include "icpRenderPassBase.h"

INCEPTION_BEGIN_NAMESPACE

#define MAX_POINT_LIGHT_COUNT 4

struct DirectionalLightRenderResource
{
	glm::vec3 direction;
	glm::vec3 ambient;
	glm::vec3 diffuse;
	glm::vec3 specular;
};

struct PointLightRenderResource
{
	glm::vec3 position;
	glm::vec3 ambient;
	glm::vec3 diffuse;
	glm::vec3 specular;
	float constant;
	float linear;
	float quadratic;
};

struct perFrameCB
{
	glm::mat4 view;
	glm::mat4 projection;
	uint32_t pointLightNumber = 0;
	DirectionalLightRenderResource dirLight;
	PointLightRenderResource pointLight[MAX_POINT_LIGHT_COUNT];
};


class icpMainForwardPass : public icpRenderPassBase
{
public:

	enum eMainForwardPassDSType : uint8_t
	{
		PER_MESH = 0,
		PER_MATERIAL,
		PER_FRAME,
		LAYOUT_TYPE_COUNT
	};

	icpMainForwardPass() = default;
	virtual ~icpMainForwardPass() override;

	void initializeRenderPass(RenderPassInitInfo initInfo) override;
	void setupPipeline() override;
	void cleanup() override;
	void render(uint32_t frameBufferIndex, uint32_t currentFrame, VkResult acquireImageResult, VkSubmitInfo& info) override;

	void createFrameBuffers();

	void createRenderPass();
	void cleanupSwapChain();
	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t curFrame);
	void recreateSwapChain() override;

	void createDescriptorSetLayouts() override;
	void allocateDescriptorSets() override;

	void CreateSceneCB();

	VkSemaphore m_waitSemaphores[1];
	VkPipelineStageFlags m_waitStages[1];

	std::vector<VkBuffer> m_perFrameCBs;
	std::vector<VmaAllocation> m_perFrameCBAllocations;

	std::vector<VkDescriptorSet> m_perFrameDSs;
	std::vector<VkDescriptorSet> m_perMeshDSs;
	std::vector<VkDescriptorSet> m_perMaterialDSs;

private:

	void updateGlobalBuffers(uint32_t curFrame);
};

INCEPTION_END_NAMESPACE