#include "icpForwardTranslucentPass.h"
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
		.SetDescriptorSetBinding(8, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.Build(m_rhi->GetLogicalDevice())
	);

	auto sceneRenderer = m_pSceneRenderer.lock();
	AddRenderpassInputLayout(sceneRenderer->GetSceneDSLayout().layout);

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

	m_pipelineInfo.m_pipeline = GraphicsPipelineBuilder(m_rhi)
		.SetVertexShader((g_system_container.m_configSystem->m_shaderFolderPath / "Translucent.vert.spv").string())
		.SetFragmentShader((g_system_container.m_configSystem->m_shaderFolderPath / "Translucent.frag.spv").string())
		.SetVertexInput({ icpVertex::getBindingDescription() }, icpVertex::getAttributeDescription())
		.SetInputAssembly(VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
		.SetPipelineLayout(dsLayouts, 0, {})
		.SetViewport({ 0.f, 0.f, static_cast<float>(m_rhi->GetSwapChainExtent().width), static_cast<float>(m_rhi->GetSwapChainExtent().height), 0.f, 1.f })
		.SetScissor({ { 0,0 }, m_rhi->GetSwapChainExtent() })
		.SetRasterizer(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE)
		.SetMultisampling(VK_SAMPLE_COUNT_1_BIT)
		.SetDepthStencilState(VK_TRUE, VK_FALSE, VK_FALSE, VK_FALSE, VK_COMPARE_OP_LESS)
		.SetColorBlendState({ attBlendState })
		.SetRenderingCreateInfo({m_rhi->GetSwapChainImageFormat()}, m_rhi->GetDepthFormat(), VK_FORMAT_UNDEFINED)
		.Build(m_pipelineInfo.m_pipelineLayout);
}

void icpForwardTranslucentPass::Render(uint32_t frameBufferIndex, uint32_t currentFrame, VkResult acquireImageResult)
{
	
}

void icpForwardTranslucentPass::RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t curFrame)
{
	auto mgr = m_pSceneRenderer.lock();
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

	std::vector<VkDeviceSize> offsets{ 0 };

	auto sceneDS = mgr->GetSceneDescriptorSet(curFrame);

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

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineInfo.m_pipeline);

		vkCmdBindDescriptorSets(commandBuffer, VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineInfo.m_pipelineLayout, 1, 1, &(meshRender.m_pMaterial->m_perMaterialDSs[curFrame]), 0, nullptr);

		auto vertBuf = meshRender.m_vertexBuffer;
		std::vector<VkBuffer>vertexBuffers{ vertBuf };

		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers.data(), offsets.data());
		vkCmdBindIndexBuffer(commandBuffer, meshRender.m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdBindDescriptorSets(commandBuffer, VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineInfo.m_pipelineLayout, 0, 1, &meshRender.m_perMeshDSs[curFrame], 0, nullptr);
		vkCmdSetPrimitiveTopology(commandBuffer, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
		vkCmdDrawIndexed(commandBuffer, meshRender.m_meshVertexIndicesNum, 1, 0, 0, 0);
	}
}


INCEPTION_END_NAMESPACE