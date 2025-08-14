#include "icpGTAOPass.h"
#include "../icpSceneRenderer.h"
#include "../../core/icpConfigSystem.h"
#include "../RHI/icpComputePipelineBuilder.h"
#include "../RHI/Vulkan/icpVulkanUtility.h"

INCEPTION_BEGIN_NAMESPACE

void icpGTAOPass::InitializeRenderPass(RenderPassInitInfo initInfo)
{
	m_pDevice = initInfo.device;
	m_pSceneRenderer = initInfo.sceneRenderer;

	AddPassInputLayout(DescriptorSetLayoutBuilder()
		.SetDescriptorSetBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)
		.SetDescriptorSetBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)
		.SetDescriptorSetBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.Build(m_pDevice->GetLogicalDevice())
	);

	auto sceneRenderer = m_pSceneRenderer.lock();
	AddPassInputLayout(sceneRenderer->GetSceneDSLayout());

	SetupPipeline();
}

void icpGTAOPass::SetupPipeline()
{
	m_pipelineInfo.m_pipeline = ComputePipelineBuilder(m_pDevice)
		.SetPipelineLayout(dsLayouts, 0, {})
		.SetComputeShader((g_system_container.m_configSystem->m_shaderFolderPath / "GTAO.comp.spv").string())
		.Build(m_pipelineInfo.m_pipelineLayout);
}

void icpGTAOPass::Dispatch(uint32_t frameBufferIndex, uint32_t currentFrame, VkResult acquireImageResult)
{
	auto renderer = m_pSceneRenderer.lock();
	auto commandBuf = renderer->GetDeferredCommandBuffer(currentFrame);

	uint32_t groupsX = ((float)m_pDevice->GetSwapChainExtent().width / 2.f + 16 - 1) / 16;
	uint32_t groupsY = ((float)m_pDevice->GetSwapChainExtent().width / 2.f + 16 - 1) / 16;
	uint32_t groupsZ = 1;

	vkCmdDispatch(commandBuf, groupsX, groupsY, groupsZ);
}

void icpGTAOPass::AllocatedRenderPassDescriptorSets()
{
	auto renderer = m_pSceneRenderer.lock();

	icpTextureRenderResourceInfo depthInfo;
	depthInfo.m_texImageViews[0] = m_pDevice->GetDepthImageView();

	GTAOPassDSs = DescriptorSetBuilder(3u)
		.SetCombinedImageSampler(0, renderer->GetGBufferBRenderResource())
		.SetCombinedImageSampler(1, depthInfo)
		.SetStorageImage(2, AORT)
		.Build(m_pDevice->GetLogicalDevice(), m_pDevice->GetDescriptorPool(), dsLayouts[0]);
}

void icpGTAOPass::SetupPassOutput()
{
	icpVulkanUtility::CreateGPUImage(
		(float)m_pDevice->GetSwapChainExtent().width / 2.f,
		(float)m_pDevice->GetSwapChainExtent().height / 2.f,
		1,
		1,
		VK_FORMAT_R8_UNORM,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
		m_pDevice->GetVmaAllocator(),
		AORT.m_texImage,
		AORT.m_texBufferAllocation
	);

	AORT.m_texImageViews[0] = icpVulkanUtility::CreateGPUImageView(
		AORT.m_texImage,
		VK_IMAGE_VIEW_TYPE_2D,
		VK_FORMAT_R8_UNORM,
		VK_IMAGE_ASPECT_COLOR_BIT,
		1,
		0,
		1,
		m_pDevice->GetLogicalDevice()
	);
}


INCEPTION_END_NAMESPACE