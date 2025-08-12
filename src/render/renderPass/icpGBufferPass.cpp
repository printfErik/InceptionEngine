#include "icpGBufferPass.h"
#include "../RHI/Vulkan/icpVulkanUtility.h"
#include "../../core/icpConfigSystem.h"
#include "../../mesh/icpMeshData.h"
#include "../icpSceneRenderer.h"
#include "../../mesh/icpMeshRendererComponent.h"
#include "../../scene/icpXFormComponent.h"
#include "../../mesh/icpPrimitiveRendererComponent.h"
#include "../../mesh/icpMeshResource.h"
#include "../RHI/icpGraphicsPipelineBuilder.h"

INCEPTION_BEGIN_NAMESPACE

static constexpr uint32_t GBUFFER_RT_COUNT = 3;

icpGBufferPass::icpGBufferPass()
{
	
}


icpGBufferPass::~icpGBufferPass()
{
	
}

void icpGBufferPass::InitializeRenderPass(RenderPassInitInfo initInfo)
{
	m_rhi = initInfo.device;
	m_pSceneRenderer = initInfo.sceneRenderer;

	AddRenderpassInputLayout(DescriptorSetLayoutBuilder()
		.SetDescriptorSetBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
		.Build(m_rhi->GetLogicalDevice())
	);

	AddRenderpassInputLayout(DescriptorSetLayoutBuilder()
		.SetDescriptorSetBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.SetDescriptorSetBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.SetDescriptorSetBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.SetDescriptorSetBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.SetDescriptorSetBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.SetDescriptorSetBinding(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.SetDescriptorSetBinding(6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.SetDescriptorSetBinding(7, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.Build(m_rhi->GetLogicalDevice())
	);

	auto sceneRenderer = m_pSceneRenderer.lock();
	AddRenderpassInputLayout(sceneRenderer->GetSceneDSLayout());

	SetupPipeline();
	SetupMaskedMeshPipeline();
}

void icpGBufferPass::Cleanup()
{
	vkDestroyPipelineLayout(m_rhi->GetLogicalDevice(), m_pipelineInfo.m_pipelineLayout, nullptr);
	vkDestroyPipeline(m_rhi->GetLogicalDevice(), m_pipelineInfo.m_pipeline, nullptr);
}

void icpGBufferPass::SetupPipeline()
{
	// Color Blend
	VkPipelineColorBlendAttachmentState attBlendState{};
	attBlendState.colorWriteMask = VkColorComponentFlagBits::VK_COLOR_COMPONENT_R_BIT
		| VkColorComponentFlagBits::VK_COLOR_COMPONENT_G_BIT
		| VkColorComponentFlagBits::VK_COLOR_COMPONENT_B_BIT
		| VkColorComponentFlagBits::VK_COLOR_COMPONENT_A_BIT;
	attBlendState.blendEnable = VK_FALSE;

	std::vector<VkPipelineColorBlendAttachmentState> attBlendStates(GBUFFER_RT_COUNT, attBlendState);

	std::vector<VkFormat> colorFormats(3, VK_FORMAT_R16G16B16A16_SFLOAT);

	auto sceneRenderer = m_pSceneRenderer.lock();

	m_pipelineInfo.m_pipeline = GraphicsPipelineBuilder(m_rhi)
		.SetVertexShader((g_system_container.m_configSystem->m_shaderFolderPath / "GBuffer.vert.spv").string())
		.SetFragmentShader((g_system_container.m_configSystem->m_shaderFolderPath / "GBuffer.frag.spv").string())
		.SetVertexInput({ icpVertex::getBindingDescription() }, icpVertex::getAttributeDescription())
		.SetInputAssembly(VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
		.SetPipelineLayout(dsLayouts, 0, {})
		.SetViewport({ 0.f, 0.f, static_cast<float>(m_rhi->GetSwapChainExtent().width), static_cast<float>(m_rhi->GetSwapChainExtent().height), 0.f, 1.f })
		.SetScissor({ { 0,0 }, m_rhi->GetSwapChainExtent() })
		.SetRasterizer(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE)
		.SetMultisampling(VK_SAMPLE_COUNT_1_BIT)
		.SetDepthStencilState(VK_TRUE, VK_TRUE, VK_FALSE, VK_FALSE, VK_COMPARE_OP_LESS)
		.SetColorBlendState(attBlendStates)
		.SetRenderingCreateInfo(colorFormats, m_rhi->GetDepthFormat(), VK_FORMAT_UNDEFINED)
		.Build(m_pipelineInfo.m_pipelineLayout);
}

void icpGBufferPass::UpdateRenderPassCB(uint32_t curFrame)
{
}

void icpGBufferPass::Render(uint32_t frameBufferIndex, uint32_t currentFrame, VkResult acquireImageResult)
{
	auto mgr = m_pSceneRenderer.lock();
	RecordCommandBuffer(mgr->GetDeferredCommandBuffer(currentFrame), frameBufferIndex, currentFrame);
}

void icpGBufferPass::BeginRenderingCreateInfo(VkCommandBuffer cmdBuf, uint32_t imageIndex)
{
	VkRenderingAttachmentInfo GBufferAAttachment = {
		.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
		.pNext = nullptr,
		.imageView = m_pSceneRenderer.lock()->GetGBufferARenderResource().m_texImageViews[0],
		.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.resolveMode = VK_RESOLVE_MODE_NONE,
		.resolveImageView = VK_NULL_HANDLE,
		.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.clearValue = (VkClearValue) {.color = {{0.0f, 0.0f, 0.0f, 1.0f}} },
	};

	VkRenderingAttachmentInfo GBufferBAttachment = {
		.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
		.pNext = nullptr,
		.imageView = m_pSceneRenderer.lock()->GetGBufferBRenderResource().m_texImageViews[0],
		.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.resolveMode = VK_RESOLVE_MODE_NONE,
		.resolveImageView = VK_NULL_HANDLE,
		.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.clearValue = (VkClearValue) {.color = {{0.0f, 0.0f, 0.0f, 1.0f}} },
	};

	VkRenderingAttachmentInfo GBufferCAttachment = {
		.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
		.pNext = nullptr,
		.imageView = m_pSceneRenderer.lock()->GetGBufferCRenderResource().m_texImageViews[0],
		.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.resolveMode = VK_RESOLVE_MODE_NONE,
		.resolveImageView = VK_NULL_HANDLE,
		.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.clearValue = (VkClearValue) {.color = {{0.0f, 0.0f, 0.0f, 1.0f}} },
	};

	VkRenderingAttachmentInfo depthAttachment = {
		.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
		.pNext = nullptr,
		.imageView = m_rhi->GetDepthImageView(),
		.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		.resolveMode = VK_RESOLVE_MODE_NONE,
		.resolveImageView = VK_NULL_HANDLE,
		.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.clearValue = (VkClearValue) {.depthStencil = {1.0f, 0} },
	};

	std::vector<VkRenderingAttachmentInfo> colorAttachments = {
		GBufferAAttachment,
		GBufferBAttachment,
		GBufferCAttachment,
	};

	VkRenderingInfo renderingInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
		.pNext = nullptr,
		.flags = 0,
		.renderArea = {.offset = {0,0}, .extent = m_rhi->GetSwapChainExtent() },
		.layerCount = 1,
		.viewMask = 0,
		.colorAttachmentCount = 3,
		.pColorAttachments = colorAttachments.data(),
		.pDepthAttachment = &depthAttachment,
		.pStencilAttachment = nullptr,
	};

	vkCmdBeginRendering(cmdBuf, &renderingInfo);
}

void icpGBufferPass::RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t curFrame)
{
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)m_rhi->GetSwapChainExtent().width;
	viewport.height = (float)m_rhi->GetSwapChainExtent().height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = m_rhi->GetSwapChainExtent();
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	auto mgr = m_pSceneRenderer.lock();
	auto SceneDS = mgr->GetSceneDescriptorSet(curFrame);
	vkCmdBindDescriptorSets(commandBuffer, VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS,
		m_pipelineInfo.m_pipelineLayout, 2, 1, &SceneDS,
		0, nullptr);

	std::vector<std::shared_ptr<icpGameEntity>> rootList;
	g_system_container.m_sceneSystem->getRootEntityList(rootList);

	for (auto entity : rootList)
	{
		Draw<icpMeshRendererComponent>(entity, commandBuffer, curFrame);
		Draw<icpPrimitiveRendererComponent>(entity, commandBuffer, curFrame);
	}
}

void icpGBufferPass::SetupMaskedMeshPipeline()
{
	auto sceneRenderer = m_pSceneRenderer.lock();

	// Color Blend
	VkPipelineColorBlendAttachmentState attBlendState{};
	attBlendState.colorWriteMask = VkColorComponentFlagBits::VK_COLOR_COMPONENT_R_BIT
		| VkColorComponentFlagBits::VK_COLOR_COMPONENT_G_BIT
		| VkColorComponentFlagBits::VK_COLOR_COMPONENT_B_BIT
		| VkColorComponentFlagBits::VK_COLOR_COMPONENT_A_BIT;
	attBlendState.blendEnable = VK_FALSE;

	std::vector<VkFormat> colorFormats(3, VK_FORMAT_R16G16B16A16_SFLOAT);

	std::vector<VkPipelineColorBlendAttachmentState> attBlendStates(GBUFFER_RT_COUNT, attBlendState);

	maskedMeshPipeline.m_pipeline = GraphicsPipelineBuilder(m_rhi)
		.SetVertexShader((g_system_container.m_configSystem->m_shaderFolderPath / "MaskedMesh.vert.spv").string())
		.SetFragmentShader((g_system_container.m_configSystem->m_shaderFolderPath / "MaskedMesh.frag.spv").string())
		.SetVertexInput({ icpVertex::getBindingDescription() }, icpVertex::getAttributeDescription())
		.SetInputAssembly(VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
		.SetPipelineLayout(dsLayouts, 0, {})
		.SetViewport({ 0.f, 0.f, static_cast<float>(m_rhi->GetSwapChainExtent().width), static_cast<float>(m_rhi->GetSwapChainExtent().height), 0.f, 1.f })
		.SetScissor({ { 0,0 }, m_rhi->GetSwapChainExtent() })
		.SetRasterizer(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE)
		.SetMultisampling(VK_SAMPLE_COUNT_1_BIT)
		.SetDepthStencilState(VK_TRUE, VK_TRUE, VK_FALSE, VK_FALSE, VK_COMPARE_OP_LESS)
		.SetColorBlendState(attBlendStates)
		.SetRenderingCreateInfo(colorFormats, m_rhi->GetDepthFormat(), VK_FORMAT_UNDEFINED)
		.Build(maskedMeshPipeline.m_pipelineLayout);
}


INCEPTION_END_NAMESPACE