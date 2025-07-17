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

	CreateDescriptorSetLayouts();
	SetupPipeline();
}

void icpGBufferPass::Cleanup()
{
	vkDestroyPipelineLayout(m_rhi->GetLogicalDevice(), m_pipelineInfo.m_pipelineLayout, nullptr);
	vkDestroyPipeline(m_rhi->GetLogicalDevice(), m_pipelineInfo.m_pipeline, nullptr);
}

void icpGBufferPass::SetupPipeline()
{
	// Pipeline Layout
	std::vector<VkDescriptorSetLayout> layouts{};
	for (auto& layoutInfo : m_DSLayouts)
	{
		layouts.push_back(layoutInfo.layout);
	}
	auto sceneRenderer = m_pSceneRenderer.lock();
	layouts.push_back(sceneRenderer->GetSceneDSLayout().layout);

	// Color Blend
	VkPipelineColorBlendAttachmentState attBlendState{};
	attBlendState.colorWriteMask = VkColorComponentFlagBits::VK_COLOR_COMPONENT_R_BIT
		| VkColorComponentFlagBits::VK_COLOR_COMPONENT_G_BIT
		| VkColorComponentFlagBits::VK_COLOR_COMPONENT_B_BIT
		| VkColorComponentFlagBits::VK_COLOR_COMPONENT_A_BIT;
	attBlendState.blendEnable = VK_FALSE;

	std::vector<VkPipelineColorBlendAttachmentState> attBlendStates(GBUFFER_RT_COUNT, attBlendState);

	m_pipelineInfo.m_pipeline = GraphicsPipelineBuilder(m_rhi, sceneRenderer->GetGBufferRenderPass(), 0)
		.SetVertexShader((g_system_container.m_configSystem->m_shaderFolderPath / "GBuffer.vert.spv").string())
		.SetFragmentShader((g_system_container.m_configSystem->m_shaderFolderPath / "GBuffer.frag.spv").string())
		.SetVertexInput({ icpVertex::getBindingDescription() }, icpVertex::getAttributeDescription())
		.SetInputAssembly(VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
		.SetPipelineLayout(layouts, 0, {})
		.SetViewport({ 0.f, 0.f, static_cast<float>(m_rhi->GetSwapChainExtent().width), static_cast<float>(m_rhi->GetSwapChainExtent().height), 0.f, 1.f })
		.SetScissor({ { 0,0 }, m_rhi->GetSwapChainExtent() })
		.SetRasterizer(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE)
		.SetMultisampling(VK_SAMPLE_COUNT_1_BIT)
		.SetDepthStencilState(VK_TRUE, VK_TRUE, VK_FALSE, VK_FALSE, VK_COMPARE_OP_LESS)
		.SetColorBlendState(attBlendStates)
		.Build(m_pipelineInfo.m_pipelineLayout);
}

void icpGBufferPass::CreateDescriptorSetLayouts()
{
	m_DSLayouts.resize(eGBufferPassDSType::LAYOUT_TYPE_COUNT);
	auto logicDevice = m_rhi->GetLogicalDevice();
	// per mesh
	{
		// set 0, binding 0 
		VkDescriptorSetLayoutBinding perObjectUBOBinding{};
		perObjectUBOBinding.binding = 0;
		perObjectUBOBinding.descriptorCount = 1;
		perObjectUBOBinding.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		perObjectUBOBinding.pImmutableSamplers = nullptr;
		perObjectUBOBinding.stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;
		m_DSLayouts[eGBufferPassDSType::PER_MESH].bindings.push_back({ VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER });

		std::array<VkDescriptorSetLayoutBinding, 1> bindings{ perObjectUBOBinding };

		VkDescriptorSetLayoutCreateInfo createInfo{};
		createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		createInfo.pBindings = bindings.data();
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

		if (vkCreateDescriptorSetLayout(logicDevice, &createInfo, nullptr, &m_DSLayouts[eGBufferPassDSType::PER_MESH].layout) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create descriptor set layout!");
		}
	}

	// per Material
	{
		// set 1, binding 0 
		VkDescriptorSetLayoutBinding perMaterialUBOBinding{};
		perMaterialUBOBinding.binding = 0;
		perMaterialUBOBinding.descriptorCount = 1;
		perMaterialUBOBinding.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		perMaterialUBOBinding.pImmutableSamplers = nullptr;
		perMaterialUBOBinding.stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
		m_DSLayouts[eGBufferPassDSType::PER_MATERIAL].bindings.push_back({ VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER });

		std::vector<VkDescriptorSetLayoutBinding> bindings {perMaterialUBOBinding};

		for (int i = 0; i < 7; i++)
		{
			// set 1, binding i
			VkDescriptorSetLayoutBinding perMaterialTextureSamplerLayoutBinding{};
			perMaterialTextureSamplerLayoutBinding.binding = i + 1;
			perMaterialTextureSamplerLayoutBinding.descriptorCount = 1;
			perMaterialTextureSamplerLayoutBinding.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			perMaterialTextureSamplerLayoutBinding.pImmutableSamplers = nullptr;
			perMaterialTextureSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			m_DSLayouts[eGBufferPassDSType::PER_MATERIAL].bindings.push_back({ VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER });

			bindings.push_back(perMaterialTextureSamplerLayoutBinding);
		}

		VkDescriptorSetLayoutCreateInfo createInfo{};
		createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		createInfo.pBindings = bindings.data();
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

		if (vkCreateDescriptorSetLayout(logicDevice, &createInfo, nullptr, &m_DSLayouts[eGBufferPassDSType::PER_MATERIAL].layout) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create descriptor set layout!");
		}
	}
}

void icpGBufferPass::UpdateRenderPassCB(uint32_t curFrame)
{
}

void icpGBufferPass::Render(uint32_t frameBufferIndex, uint32_t currentFrame, VkResult acquireImageResult)
{
	auto mgr = m_pSceneRenderer.lock();
	//vkResetCommandBuffer(mgr->m_vMainForwardCommandBuffers[currentFrame], 0);
	RecordCommandBuffer(mgr->GetDeferredCommandBuffer(currentFrame), frameBufferIndex, currentFrame);
}

void icpGBufferPass::RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t curFrame)
{
	auto mgr = m_pSceneRenderer.lock();

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineInfo.m_pipeline);

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
	vkCmdBindDescriptorSets(commandBuffer, VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineInfo.m_pipelineLayout, 2, 1, &sceneDS, 0, nullptr);

	std::vector<std::shared_ptr<icpGameEntity>> rootList;
	g_system_container.m_sceneSystem->getRootEntityList(rootList);

	for (auto entity : rootList)
	{
		if (entity->hasComponent<icpMeshRendererComponent>())
		{
			const auto& meshRender = entity->accessComponent<icpMeshRendererComponent>();

			if (meshRender.m_pMaterial->m_shadingModel != eMaterialShadingModel::PBR_LIT)
			{
				continue;
			}

			vkCmdBindDescriptorSets(commandBuffer, VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineInfo.m_pipelineLayout, 1, 1, &(meshRender.m_pMaterial->m_perMaterialDSs[curFrame]), 0, nullptr);

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

			if (primitive.m_pMaterial->m_shadingModel != eMaterialShadingModel::PBR_LIT)
			{
				continue;
			}

			vkCmdBindDescriptorSets(commandBuffer, VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineInfo.m_pipelineLayout, 1, 1, &(primitive.m_pMaterial->m_perMaterialDSs[curFrame]), 0, nullptr);

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

void icpGBufferPass::SetupMaskedMeshPipeline()
{
	// Pipeline Layout
	std::vector<VkDescriptorSetLayout> layouts{};
	for (auto& layoutInfo : m_DSLayouts)
	{
		layouts.push_back(layoutInfo.layout);
	}
	auto sceneRenderer = m_pSceneRenderer.lock();
	layouts.push_back(sceneRenderer->GetSceneDSLayout().layout);

	// Color Blend
	VkPipelineColorBlendAttachmentState attBlendState{};
	attBlendState.colorWriteMask = VkColorComponentFlagBits::VK_COLOR_COMPONENT_R_BIT
		| VkColorComponentFlagBits::VK_COLOR_COMPONENT_G_BIT
		| VkColorComponentFlagBits::VK_COLOR_COMPONENT_B_BIT
		| VkColorComponentFlagBits::VK_COLOR_COMPONENT_A_BIT;
	attBlendState.blendEnable = VK_FALSE;

	std::vector<VkPipelineColorBlendAttachmentState> attBlendStates(GBUFFER_RT_COUNT, attBlendState);

	maskedMeshPipeline.m_pipeline = GraphicsPipelineBuilder(m_rhi, sceneRenderer->GetGBufferRenderPass(), 0)
		.SetVertexShader((g_system_container.m_configSystem->m_shaderFolderPath / "MaskedMeshPass.vert.spv").string())
		.SetFragmentShader((g_system_container.m_configSystem->m_shaderFolderPath / "MaskedMeshPass.frag.spv").string())
		.SetVertexInput({ icpVertex::getBindingDescription() }, icpVertex::getAttributeDescription())
		.SetInputAssembly(VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
		.SetPipelineLayout(layouts, 0, {})
		.SetViewport({ 0.f, 0.f, static_cast<float>(m_rhi->GetSwapChainExtent().width), static_cast<float>(m_rhi->GetSwapChainExtent().height), 0.f, 1.f })
		.SetScissor({ { 0,0 }, m_rhi->GetSwapChainExtent() })
		.SetRasterizer(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE)
		.SetMultisampling(VK_SAMPLE_COUNT_1_BIT)
		.SetDepthStencilState(VK_TRUE, VK_TRUE, VK_FALSE, VK_FALSE, VK_COMPARE_OP_LESS)
		.SetColorBlendState(attBlendStates)
		.Build(maskedMeshPipeline.m_pipelineLayout);
}


INCEPTION_END_NAMESPACE