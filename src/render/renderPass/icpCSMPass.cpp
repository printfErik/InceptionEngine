#include "icpCSMPass.h"

#include "../icpRenderSystem.h"
#include "../../core/icpConfigSystem.h"
#include "../RHI/Vulkan/icpVulkanUtility.h"
#include "../../mesh/icpMeshData.h"
#include "../icpSceneRenderer.h"
#include "../shadow/icpShadowManager.h"
#include "../../mesh/icpMeshRendererComponent.h"
#include "../../mesh/icpPrimitiveRendererComponent.h"
#include "../../core/icpLogSystem.h"
#include "../RHI/icpGraphicsPipelineBuilder.h"

INCEPTION_BEGIN_NAMESPACE

icpCSMPass::icpCSMPass()
{

}


icpCSMPass::~icpCSMPass()
{
	
}

void icpCSMPass::Cleanup()
{
	vkDestroyPipelineLayout(m_rhi->GetLogicalDevice(), m_pipelineInfo.m_pipelineLayout, nullptr);
	vkDestroyPipeline(m_rhi->GetLogicalDevice(), m_pipelineInfo.m_pipeline, nullptr);
}


void icpCSMPass::InitializeRenderPass(RenderPassInitInfo initInfo)
{
	m_rhi = initInfo.device;
	m_pSceneRenderer = initInfo.sceneRenderer;

	CreateCSMImageRenderResource();
	CreateCSMRenderPass();
	CreateCSMFrameBuffer();

	AddRenderpassInputLayout(DescriptorSetLayoutBuilder()
		.SetDescriptorSetBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
		.Build(m_rhi->GetLogicalDevice())
	);

	// csm projview matrix DS layout 
	AddRenderpassInputLayout(DescriptorSetLayoutBuilder()
		.SetDescriptorSetBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
		.Build(m_rhi->GetLogicalDevice())
	);

	SetupPipeline();
}

void icpCSMPass::CreateCSMRenderPass()
{
	std::array<VkAttachmentDescription, 1> attachments{};

	// Depth attachment
	attachments[0].format = m_rhi->GetDepthFormat();
	attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkAttachmentReference depthReference = { 0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

	VkSubpassDescription subpassDescription{};
	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescription.colorAttachmentCount = 0;
	subpassDescription.pColorAttachments = nullptr;
	subpassDescription.pDepthStencilAttachment = &depthReference;

	std::array<VkSubpassDependency, 2> dependency;
	dependency[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency[0].dstSubpass = 0;
	dependency[0].srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	dependency[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency[0].srcAccessMask = 0;
	dependency[0].dstAccessMask =  VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependency[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependency[1].srcSubpass = 0;
	dependency[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependency[1].srcStageMask =  VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	dependency[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependency[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependency[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	dependency[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	VkRenderPassCreateInfo shadowPassInfo{};
	shadowPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	shadowPassInfo.attachmentCount = attachments.size();
	shadowPassInfo.pAttachments = attachments.data();
	shadowPassInfo.dependencyCount = 2;
	shadowPassInfo.pDependencies = dependency.data();
	shadowPassInfo.subpassCount = 1;
	shadowPassInfo.pSubpasses = &subpassDescription;

	if (vkCreateRenderPass(m_rhi->GetLogicalDevice(), &shadowPassInfo, nullptr, &m_shadowRenderPass) != VK_SUCCESS)
	{
		ICP_LOG_FATAL("failed to create cascade shadow map render pass!");
	}
}

void icpCSMPass::CreateCSMImageRenderResource()
{
	icpVulkanUtility::CreateGPUImage(
		s_cascadeShadowMapResolution,
		s_cascadeShadowMapResolution,
		1,
		s_csmCascadeCount,
		m_rhi->GetDepthFormat(),
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		m_rhi->GetVmaAllocator(),
		m_csmArray, m_csmArrayAllocation
	);

	m_csmArrayViews.resize(s_csmCascadeCount + 1);

	for (uint32_t i = 0; i < s_csmCascadeCount; i++)
	{
		m_csmArrayViews[i] = icpVulkanUtility::CreateGPUImageView(
			m_csmArray,
			VK_IMAGE_VIEW_TYPE_2D_ARRAY,
			m_rhi->GetDepthFormat(),
			VK_IMAGE_ASPECT_DEPTH_BIT,
			1,
			i,
			1,
			m_rhi->GetLogicalDevice()
		);
	}

	m_csmArrayViews[s_csmCascadeCount] = icpVulkanUtility::CreateGPUImageView(
		m_csmArray,
		VK_IMAGE_VIEW_TYPE_2D_ARRAY,
		m_rhi->GetDepthFormat(),
		VK_IMAGE_ASPECT_DEPTH_BIT,
		1,
		0,
		s_csmCascadeCount,
		m_rhi->GetLogicalDevice()
	);
	
}

void icpCSMPass::CreateCSMFrameBuffer()
{
	size_t swapChainImageViewSize = m_rhi->GetSwapChainImageViews().size();
	m_csmFrameBuffers.resize(swapChainImageViewSize * s_csmCascadeCount);

	for (size_t i = 0; i < swapChainImageViewSize; i++)
	{
		for (uint32_t cascade = 0; cascade < s_csmCascadeCount; cascade++)
		{
			std::array<VkImageView, 1> attachment =
			{
				m_csmArrayViews[cascade]
			};

			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = m_shadowRenderPass;
			framebufferInfo.attachmentCount = static_cast<uint32_t>(attachment.size());
			framebufferInfo.pAttachments = attachment.data();
			framebufferInfo.width = s_cascadeShadowMapResolution;
			framebufferInfo.height = s_cascadeShadowMapResolution;
			framebufferInfo.layers = 1;

			if (vkCreateFramebuffer(m_rhi->GetLogicalDevice(), &framebufferInfo, nullptr, &m_csmFrameBuffers[i * s_csmCascadeCount + cascade]) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create csm frame buffer!");
			}
		}
	}
}

void icpCSMPass::SetupPipeline()
{

	VkPushConstantRange pushRange{};
	pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	pushRange.offset = 0;
	pushRange.size = sizeof(glm::mat4);

	m_pipelineInfo.m_pipeline = GraphicsPipelineBuilder(m_rhi)
		.SetVertexShader((g_system_container.m_configSystem->m_shaderFolderPath / "CSM.vert.spv").string())
		.SetVertexInput({ icpVertex::getBindingDescription() }, icpVertex::getAttributeDescription())
		.SetInputAssembly(VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
		.SetPipelineLayout(dsLayouts, 1, pushRange)
		.SetViewport({ 0.f, 0.f, s_cascadeShadowMapResolution, s_cascadeShadowMapResolution, 0.f, 1.f })
		.SetScissor({ { 0,0 }, {s_cascadeShadowMapResolution, s_cascadeShadowMapResolution} })
		.SetRasterizer(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_TRUE, 1.25f, 1.75f, 0.f)
		.SetMultisampling(VK_SAMPLE_COUNT_1_BIT)
		.SetDepthStencilState(VK_TRUE, VK_TRUE, VK_FALSE, VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL)
		.SetColorBlendState({})
		.SetRenderPass(m_shadowRenderPass, 0)
		.Build(m_pipelineInfo.m_pipelineLayout);
}

void icpCSMPass::Render(uint32_t frameBufferIndex, uint32_t currentFrame, VkResult acquireImageResult)
{
	
}

void icpCSMPass::RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t curFrame)
{
	
}



void icpCSMPass::RenderPushConstant(uint32_t frameBufferIndex, uint32_t currentFrame, uint32_t cascadeIndex, VkResult acquireImageResult)
{
	auto mgr = m_pSceneRenderer.lock();
	RecordCommandBufferPushConstant(mgr->GetDeferredCommandBuffer(currentFrame), frameBufferIndex, cascadeIndex, currentFrame);
}

void icpCSMPass::RecordCommandBufferPushConstant(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t cascadeIndex, uint32_t curFrame)
{
	auto sceneRenderer = m_pSceneRenderer.lock();

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineInfo.m_pipeline);
	
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = s_cascadeShadowMapResolution;
	viewport.height = s_cascadeShadowMapResolution;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = { s_cascadeShadowMapResolution, s_cascadeShadowMapResolution };
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
	
	auto shadowMgr = g_system_container.m_renderSystem->m_shadowManager;

	vkCmdBindDescriptorSets(commandBuffer, 
		VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, 
		m_pipelineInfo.m_pipelineLayout, 
		1, 1, &shadowMgr->m_csmDSs[curFrame], 0, nullptr
	);

	vkCmdPushConstants(
		commandBuffer,
		m_pipelineInfo.m_pipelineLayout,
		VK_SHADER_STAGE_VERTEX_BIT,
		0,
		sizeof(glm::mat4),
		&shadowMgr->m_lightProjViews[cascadeIndex]
	);

	std::vector<std::shared_ptr<icpGameEntity>> rootList;
	g_system_container.m_sceneSystem->getRootEntityList(rootList);
	std::vector<VkDeviceSize> offsets{ 0 };

	for (auto entity : rootList)
	{
		if (entity->hasComponent<icpMeshRendererComponent>())
		{
			const auto& meshRender = entity->accessComponent<icpMeshRendererComponent>();

			auto vertBuf = meshRender.m_vertexBuffer;
			std::vector<VkBuffer>vertexBuffers{ vertBuf };

			vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers.data(), offsets.data());
			vkCmdBindIndexBuffer(commandBuffer, meshRender.m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);

			vkCmdBindDescriptorSets(commandBuffer, VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineInfo.m_pipelineLayout, 0, 1, &meshRender.m_perMeshDSs[curFrame], 0, nullptr);
			vkCmdSetPrimitiveTopology(commandBuffer, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
			vkCmdDrawIndexed(commandBuffer, meshRender.m_meshVertexIndicesNum, 1, 0, 0, 0);
		}
		else if (entity->hasComponent<icpPrimitiveRendererComponent>())
		{
			auto& primitive = entity->accessComponent<icpPrimitiveRendererComponent>();

			auto vertBuf = primitive.m_vertexBuffer;
			std::vector<VkBuffer>vertexBuffers{ vertBuf };

			vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers.data(), offsets.data());
			vkCmdBindIndexBuffer(commandBuffer, primitive.m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);

			auto& descriptorSets = primitive.m_descriptorSets;
			vkCmdBindDescriptorSets(commandBuffer, VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineInfo.m_pipelineLayout, 0, 1, &descriptorSets[curFrame], 0, nullptr);

			vkCmdSetPrimitiveTopology(commandBuffer, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);

			vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(primitive.m_vertexIndices.size()), 1, 0, 0, 0);
		}
	}
}

void icpCSMPass::BeginCSMRenderPass(uint32_t imageIndex, uint32_t cascade, VkCommandBuffer& commandBuffer)
{
	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = m_shadowRenderPass;
	renderPassInfo.framebuffer = m_csmFrameBuffers[imageIndex * s_csmCascadeCount + cascade];
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = {s_cascadeShadowMapResolution, s_cascadeShadowMapResolution};

	VkClearValue clearVal{};
	clearVal.depthStencil = { 1.0f, 0 };

	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = &clearVal;

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void icpCSMPass::EndCSMRenderPass(VkCommandBuffer& commandBuffer)
{
	vkCmdEndRenderPass(commandBuffer);
}

INCEPTION_END_NAMESPACE