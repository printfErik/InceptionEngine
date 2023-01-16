#pragma once
#include "../../core/icpMacros.h"
#include "icpRenderPassBase.h"

INCEPTION_BEGIN_NAMESPACE

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
	float constant;
	float linear;
	float quadratic;

	glm::vec3 ambient;
	glm::vec3 diffuse;
	glm::vec3 specular;
};

struct SSBOPerFrame
{
	glm::mat4 view;
	glm::mat4 projection;
	DirectionalLightRenderResource dirLight;
	PointLightRenderResource pointLight[16];
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

	void initializeRenderPass(RendePassInitInfo initInfo) override;
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

	VkSemaphore m_waitSemaphores[1];
	VkPipelineStageFlags m_waitStages[1];

	VkBuffer m_SSBOPerFrame{VK_NULL_HANDLE};
	VkDescriptorSet m_perFrameDS;

private:

	void updateMeshUniformBuffers(uint32_t curFrame);
};

INCEPTION_END_NAMESPACE