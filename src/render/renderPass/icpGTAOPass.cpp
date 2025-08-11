#include "icpGTAOPass.h"
#include "../icpSceneRenderer.h"

INCEPTION_BEGIN_NAMESPACE

void icpGTAOPass::InitializeRenderPass(RenderPassInitInfo initInfo)
{
	m_rhi = initInfo.device;
	m_pSceneRenderer = initInfo.sceneRenderer;

	AddRenderpassInputLayout(DescriptorSetLayoutBuilder()
		.SetDescriptorSetBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)
		.SetDescriptorSetBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)
		.Build(m_rhi->GetLogicalDevice())
	);

	auto sceneRenderer = m_pSceneRenderer.lock();
	AddRenderpassInputLayout(sceneRenderer->GetSceneDSLayout());

	SetupPipeline();
}

void icpGTAOPass::SetupPipeline()
{
}

void icpGTAOPass::Dispatch(uint32_t frameBufferIndex, uint32_t currentFrame, VkResult acquireImageResult)
{


}


INCEPTION_END_NAMESPACE