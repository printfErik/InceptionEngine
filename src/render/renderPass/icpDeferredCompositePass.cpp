#include "icpDeferredCompositePass.h"

#include "icpCSMPass.h"
#include "../RHI/Vulkan/icpVulkanUtility.h"
#include "../../core/icpConfigSystem.h"
#include "../../mesh/icpMeshData.h"
#include "../icpSceneRenderer.h"
#include "../material/icpImageSampler.h"
#include "../shadow/icpShadowManager.h"
#include "../../render/icpRenderSystem.h"
#include "../RHI/icpGraphicsPipelineBuilder.h"

INCEPTION_BEGIN_NAMESPACE

icpDeferredCompositePass::icpDeferredCompositePass()
{
	
}

icpDeferredCompositePass::~icpDeferredCompositePass()
{
}

void icpDeferredCompositePass::Cleanup()
{
	vkDestroyPipelineLayout(m_rhi->GetLogicalDevice(), m_pipelineInfo.m_pipelineLayout, nullptr);
	vkDestroyPipeline(m_rhi->GetLogicalDevice(), m_pipelineInfo.m_pipeline, nullptr);
}

void icpDeferredCompositePass::Render(uint32_t frameBufferIndex, uint32_t currentFrame, VkResult acquireImageResult)
{
	auto mgr = m_pSceneRenderer.lock();
	RecordCommandBuffer(mgr->GetDeferredCommandBuffer(currentFrame), frameBufferIndex, currentFrame);
}

void icpDeferredCompositePass::RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t curFrame)
{
	auto renderer = m_pSceneRenderer.lock();

	vkCmdNextSubpass(commandBuffer, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineInfo.m_pipeline);

	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
		m_pipelineInfo.m_pipelineLayout, 0, 1, &m_vGBufferDSs[curFrame], 0, nullptr);

	auto sceneDs = renderer->GetSceneDescriptorSet(curFrame);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
		m_pipelineInfo.m_pipelineLayout, 2, 1, &sceneDs, 0, nullptr);

	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
		m_pipelineInfo.m_pipelineLayout, 1, 1, &m_csmDSs[curFrame], 0, nullptr);

	vkCmdDraw(commandBuffer, 3, 1, 0, 0);
}

void icpDeferredCompositePass::UpdateRenderPassCB(uint32_t curFrame)
{
	
}

void icpDeferredCompositePass::InitializeRenderPass(RenderPassInitInfo initInfo)
{
	m_rhi = initInfo.device;
	m_pSceneRenderer = initInfo.sceneRenderer;

	AddRenderpassInputLayout(DescriptorSetLayoutBuilder()
		.SetDescriptorSetBinding(0, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT)
		.SetDescriptorSetBinding(1, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT)
		.SetDescriptorSetBinding(2, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT)
		.SetDescriptorSetBinding(3, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT)
		.Build(m_rhi->GetLogicalDevice())
	);

	AddRenderpassInputLayout(DescriptorSetLayoutBuilder()
		.SetDescriptorSetBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.SetDescriptorSetBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.Build(m_rhi->GetLogicalDevice())
	);

	auto sceneRenderer = m_pSceneRenderer.lock();
	AddRenderpassInputLayout(sceneRenderer->GetSceneDSLayout().layout);
	
	SetupPipeline();
}

void icpDeferredCompositePass::SetupPipeline()
{
	auto sceneRenderer = m_pSceneRenderer.lock();

	VkPipelineColorBlendAttachmentState attBlendState{};
	attBlendState.colorWriteMask = VkColorComponentFlagBits::VK_COLOR_COMPONENT_R_BIT
		| VkColorComponentFlagBits::VK_COLOR_COMPONENT_G_BIT
		| VkColorComponentFlagBits::VK_COLOR_COMPONENT_B_BIT
		| VkColorComponentFlagBits::VK_COLOR_COMPONENT_A_BIT;
	attBlendState.blendEnable = VK_FALSE;

	m_pipelineInfo.m_pipeline = GraphicsPipelineBuilder(m_rhi)
		.SetVertexShader((g_system_container.m_configSystem->m_shaderFolderPath / "DeferredComposite.vert.spv").string())
		.SetFragmentShader((g_system_container.m_configSystem->m_shaderFolderPath / "DeferredComposite.frag.spv").string())
		.SetVertexInput({ icpVertex::getBindingDescription() }, icpVertex::getAttributeDescription())
		.SetInputAssembly(VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
		.SetPipelineLayout(dsLayouts, 0, {})
		.SetViewport({ 0.f, 0.f, static_cast<float>(m_rhi->GetSwapChainExtent().width), static_cast<float>(m_rhi->GetSwapChainExtent().height), 0.f, 1.f })
		.SetScissor({ { 0,0 }, m_rhi->GetSwapChainExtent() })
		.SetRasterizer(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE)
		.SetMultisampling(VK_SAMPLE_COUNT_1_BIT)
		.SetDepthStencilState(VK_TRUE, VK_FALSE, VK_FALSE, VK_FALSE, VK_COMPARE_OP_ALWAYS)
		.SetColorBlendState({ attBlendState })
		.SetRenderPass(sceneRenderer->GetGBufferRenderPass(), 1)
		.Build(m_pipelineInfo.m_pipelineLayout);
}
INCEPTION_END_NAMESPACE