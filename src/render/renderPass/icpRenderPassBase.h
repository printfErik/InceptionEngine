#include "../../core/icpMacros.h"
#include "../icpVulkanRHI.h"

#include <vulkan/vulkan.hpp>

INCEPTION_BEGIN_NAMESPACE

enum class eRenderPass
{
	MAIN_FORWARD_PASS = 0,
	UI_PASS,
	RENDER_PASS_COUNT
};

class icpRenderPassBase
{
public:

	struct RendePassInitInfo
	{
		std::shared_ptr<icpVulkanRHI> rhi;
	};

	struct RenederPipelineInfo
	{
		VkPipeline m_pipeline;
		VkPipelineLayout m_pipelineLayout;
	};

	icpRenderPassBase();
	virtual ~icpRenderPassBase();

	virtual void initializeRenderPass(RendePassInitInfo initInfo) = 0;
	virtual void setupPipeline() = 0;

protected:
	VkFramebuffer m_frameBuffer;
	VkRenderPass m_renderPassObj;

	RenederPipelineInfo m_pipelineInfo;

	std::shared_ptr<icpVulkanRHI> m_rhi;
};

INCEPTION_END_NAMESPACE