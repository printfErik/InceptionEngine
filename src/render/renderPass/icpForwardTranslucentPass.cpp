#include "icpForwardTranslucentPass.h"

#include "icpDeferredCompositePass.h"
#include "../../core/icpConfigSystem.h"
#include "../RHI/icpGraphicsPipelineBuilder.h"
#include "../icpSceneRenderer.h"
#include "../../mesh/icpMeshData.h"
#include "../../mesh/icpMeshRendererComponent.h"
INCEPTION_BEGIN_NAMESPACE

icpForwardTranslucentPass::~icpForwardTranslucentPass()
{
	
}


void icpForwardTranslucentPass::InitializeRenderPass(RenderPassInitInfo initInfo)
{
	m_pDevice = initInfo.device;
	m_pSceneRenderer = initInfo.sceneRenderer;

	AddPassInputLayout(DescriptorSetLayoutBuilder()
		.SetDescriptorSetBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
		.Build(m_pDevice->GetLogicalDevice())
	);

	AddPassInputLayout(DescriptorSetLayoutBuilder()
		.SetDescriptorSetBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.SetDescriptorSetBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.SetDescriptorSetBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.SetDescriptorSetBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.SetDescriptorSetBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.SetDescriptorSetBinding(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.SetDescriptorSetBinding(6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.SetDescriptorSetBinding(7, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.SetDescriptorSetBinding(8, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.Build(m_pDevice->GetLogicalDevice())
	);

	auto sceneRenderer = m_pSceneRenderer.lock();
	AddPassInputLayout(sceneRenderer->GetSceneDSLayout());

	SetupPipeline();
}

void icpForwardTranslucentPass::SetupPipeline()
{

	// Color Blend
	VkPipelineColorBlendAttachmentState attBlendState{};
	attBlendState.colorWriteMask = VkColorComponentFlagBits::VK_COLOR_COMPONENT_R_BIT
		| VkColorComponentFlagBits::VK_COLOR_COMPONENT_G_BIT
		| VkColorComponentFlagBits::VK_COLOR_COMPONENT_B_BIT
		| VkColorComponentFlagBits::VK_COLOR_COMPONENT_A_BIT;
	attBlendState.blendEnable = VK_TRUE;
	attBlendState.alphaBlendOp = VkBlendOp::VK_BLEND_OP_ADD;
	attBlendState.srcAlphaBlendFactor = VkBlendFactor::VK_BLEND_FACTOR_SRC_ALPHA;
	attBlendState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;

	m_pipelineInfo.m_pipeline = GraphicsPipelineBuilder(m_pDevice)
		.SetVertexShader((g_system_container.m_configSystem->m_shaderFolderPath / "Translucent.vert.spv").string())
		.SetFragmentShader((g_system_container.m_configSystem->m_shaderFolderPath / "Translucent.frag.spv").string())
		.SetVertexInput({ icpVertex::getBindingDescription() }, icpVertex::getAttributeDescription())
		.SetInputAssembly(VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
		.SetPipelineLayout(dsLayouts, 0, {})
		.SetViewport({ 0.f, 0.f, static_cast<float>(m_pDevice->GetSwapChainExtent().width), static_cast<float>(m_pDevice->GetSwapChainExtent().height), 0.f, 1.f })
		.SetScissor({ { 0,0 }, m_pDevice->GetSwapChainExtent() })
		.SetRasterizer(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE)
		.SetMultisampling(VK_SAMPLE_COUNT_1_BIT)
		.SetDepthStencilState(VK_TRUE, VK_FALSE, VK_FALSE, VK_FALSE, VK_COMPARE_OP_LESS)
		.SetColorBlendState({ attBlendState })
		.SetRenderingCreateInfo({m_pDevice->GetSwapChainImageFormat()}, m_pDevice->GetDepthFormat(), VK_FORMAT_UNDEFINED)
		.Build(m_pipelineInfo.m_pipelineLayout);
}

void icpForwardTranslucentPass::Render(uint32_t frameBufferIndex, uint32_t currentFrame, VkResult acquireImageResult)
{
	
}

void icpForwardTranslucentPass::RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t curFrame)
{
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineInfo.m_pipeline);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)m_pDevice->GetSwapChainExtent().width;
	viewport.height = (float)m_pDevice->GetSwapChainExtent().height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = m_pDevice->GetSwapChainExtent();
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
		const auto& meshRender = entity->accessComponent<icpMeshRendererComponent>();

		if (meshRender.m_pMaterial->m_shadingModel != eMaterialShadingModel::PBR_LIT
			|| meshRender.m_pMaterial->m_blendMode != eMaterialBlendMode::TRANSLUCENT)
		{
			continue;
		}

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
			m_pipelineInfo.m_pipelineLayout, 0, 1,
			&meshRender.MeshDSs[curFrame], 0, nullptr);

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
			m_pipelineInfo.m_pipelineLayout, 1, 1,
			&meshRender.m_pMaterial->MaterialDSs[curFrame], 0, nullptr);

		VkDeviceSize offsets = 0;
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &meshRender.MeshVB.buffer, &offsets);
		vkCmdBindIndexBuffer(commandBuffer, meshRender.MeshIB.buffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdSetPrimitiveTopology(commandBuffer, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
		vkCmdDrawIndexed(commandBuffer, meshRender.m_meshVertexIndicesNum, 1, 0, 0, 0);
	}
}

void icpForwardTranslucentPass::Cleanup()
{
	
}

void icpForwardTranslucentPass::UpdateRenderPassCB(uint32_t curFrame)
{
	
}

void icpForwardTranslucentPass::BeginRenderingCreateInfo(VkCommandBuffer cmdBuf, uint32_t imageIndex)
{
	auto lightingPass = std::static_pointer_cast<icpDeferredCompositePass>(m_renderPassDependencies[0].lock());

	VkRenderingAttachmentInfo finalColorAAttachment = {
		.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
		.pNext = nullptr,
		.imageView = lightingPass->LightingPassRT.m_texImageViews[0],
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
		.imageView = m_pDevice->GetDepthImageView(),
		.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		.resolveMode = VK_RESOLVE_MODE_NONE,
		.resolveImageView = VK_NULL_HANDLE,
		.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.clearValue = (VkClearValue) {.depthStencil = {1.0f, 0} },
	};

	VkRenderingInfo renderingInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
		.pNext = nullptr,
		.flags = 0,
		.renderArea = {.offset = {0,0}, .extent = m_pDevice->GetSwapChainExtent() },
		.layerCount = 1,
		.viewMask = 0,
		.colorAttachmentCount = 1,
		.pColorAttachments = &finalColorAAttachment,
		.pDepthAttachment = &depthAttachment,
		.pStencilAttachment = nullptr,
	};

	vkCmdBeginRendering(cmdBuf, &renderingInfo);
}


INCEPTION_END_NAMESPACE