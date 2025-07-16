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

	CreateDescriptorSetLayouts();
	AllocateDescriptorSets();
	SetupPipeline();
}

void icpDeferredCompositePass::SetupPipeline()
{
	std::vector<VkDescriptorSetLayout> layouts{};
	for (auto& layoutInfo : m_DSLayouts)
	{
		layouts.push_back(layoutInfo.layout);
	}
	auto sceneRenderer = m_pSceneRenderer.lock();
	layouts.push_back(sceneRenderer->GetSceneDSLayout().layout);

	VkPipelineColorBlendAttachmentState attBlendState{};
	attBlendState.colorWriteMask = VkColorComponentFlagBits::VK_COLOR_COMPONENT_R_BIT
		| VkColorComponentFlagBits::VK_COLOR_COMPONENT_G_BIT
		| VkColorComponentFlagBits::VK_COLOR_COMPONENT_B_BIT
		| VkColorComponentFlagBits::VK_COLOR_COMPONENT_A_BIT;
	attBlendState.blendEnable = VK_FALSE;

	m_pipelineInfo.m_pipeline = GraphicsPipelineBuilder(m_rhi, sceneRenderer->GetGBufferRenderPass(), 1)
		.SetVertexShader((g_system_container.m_configSystem->m_shaderFolderPath / "DeferredComposite.vert.spv").string())
		.SetFragmentShader((g_system_container.m_configSystem->m_shaderFolderPath / "DeferredComposite.frag.spv").string())
		.SetVertexInput({ icpVertex::getBindingDescription() }, icpVertex::getAttributeDescription())
		.SetInputAssembly(VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
		.SetPipelineLayout(layouts, 0, {})
		.SetViewport({ 0.f, 0.f, static_cast<float>(m_rhi->GetSwapChainExtent().width), static_cast<float>(m_rhi->GetSwapChainExtent().height), 0.f, 1.f })
		.SetScissor({ { 0,0 }, m_rhi->GetSwapChainExtent() })
		.SetRasterizer(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE)
		.SetMultisampling(VK_SAMPLE_COUNT_1_BIT)
		.SetDepthStencilState(VK_TRUE, VK_FALSE, VK_FALSE, VK_FALSE, VK_COMPARE_OP_ALWAYS)
		.SetColorBlendState({ attBlendState })
		.Build(m_pipelineInfo.m_pipelineLayout);
}

void icpDeferredCompositePass::CreateDescriptorSetLayouts()
{
	m_DSLayouts.resize(eDeferredCompositePassDSType::LAYOUT_TYPE_COUNT);
	auto logicDevice = m_rhi->GetLogicalDevice();

	// gbuffer
	{
		// set 0, binding 0 
		VkDescriptorSetLayoutBinding gbufferABinding{};
		gbufferABinding.binding = 0;
		gbufferABinding.descriptorCount = 1;
		gbufferABinding.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		gbufferABinding.pImmutableSamplers = nullptr;
		gbufferABinding.stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
		m_DSLayouts[eDeferredCompositePassDSType::GBUFFER].bindings.push_back({ VkDescriptorType::VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT });

		// set 0, binding 1 
		VkDescriptorSetLayoutBinding gbufferBBinding{};
		gbufferBBinding.binding = 1;
		gbufferBBinding.descriptorCount = 1;
		gbufferBBinding.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		gbufferBBinding.pImmutableSamplers = nullptr;
		gbufferBBinding.stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
		m_DSLayouts[eDeferredCompositePassDSType::GBUFFER].bindings.push_back({ VkDescriptorType::VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT });

		// set 0, binding 2
		VkDescriptorSetLayoutBinding gbufferCBinding{};
		gbufferCBinding.binding = 2;
		gbufferCBinding.descriptorCount = 1;
		gbufferCBinding.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		gbufferCBinding.pImmutableSamplers = nullptr;
		gbufferCBinding.stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
		m_DSLayouts[eDeferredCompositePassDSType::GBUFFER].bindings.push_back({ VkDescriptorType::VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT });

		// set 0, binding 3
		VkDescriptorSetLayoutBinding depthBinding{};
		depthBinding.binding = 3;
		depthBinding.descriptorCount = 1;
		depthBinding.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		depthBinding.pImmutableSamplers = nullptr;
		depthBinding.stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
		m_DSLayouts[eDeferredCompositePassDSType::GBUFFER].bindings.push_back({ VkDescriptorType::VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT });
		  
		std::array<VkDescriptorSetLayoutBinding, 4> bindings{ gbufferABinding, gbufferBBinding, gbufferCBinding, depthBinding };

		VkDescriptorSetLayoutCreateInfo createInfo{};
		createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		createInfo.pBindings = bindings.data();
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

		if (vkCreateDescriptorSetLayout(logicDevice, &createInfo, nullptr, &m_DSLayouts[eDeferredCompositePassDSType::GBUFFER].layout) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create descriptor set layout!");
		}
	}

	// CSM 
	{
		// set 1, binding 0
		VkDescriptorSetLayoutBinding CascadeSplitUBO{};
		CascadeSplitUBO.binding = 0;
		CascadeSplitUBO.descriptorCount = 1;
		CascadeSplitUBO.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		CascadeSplitUBO.pImmutableSamplers = nullptr;
		CascadeSplitUBO.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		m_DSLayouts[eDeferredCompositePassDSType::CSM].bindings.push_back({ VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER });

		// set 1, binding 1
		VkDescriptorSetLayoutBinding CascadeShadowMap{};
		CascadeShadowMap.binding = 1;
		CascadeShadowMap.descriptorCount = 1;
		CascadeShadowMap.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		CascadeShadowMap.pImmutableSamplers = nullptr;
		CascadeShadowMap.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		m_DSLayouts[eDeferredCompositePassDSType::CSM].bindings.push_back({ VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER });

		std::array<VkDescriptorSetLayoutBinding, 2> bindings{ CascadeSplitUBO, CascadeShadowMap };

		VkDescriptorSetLayoutCreateInfo createInfo{};
		createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		createInfo.pBindings = bindings.data();
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

		if (vkCreateDescriptorSetLayout(logicDevice, &createInfo, nullptr, &m_DSLayouts[eDeferredCompositePassDSType::CSM].layout) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create descriptor set layout!");
		}
	}
}

void icpDeferredCompositePass::AllocateDescriptorSets()
{
	icpDescriptorSetCreation creation{};
	creation.layoutInfo = m_DSLayouts[eDeferredCompositePassDSType::GBUFFER];

	std::vector<icpTextureRenderResourceInfo> gbufferAInfos;
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		icpTextureRenderResourceInfo texInfo{};
		texInfo.m_texSampler = VK_NULL_HANDLE;
		texInfo.m_texImageView = m_pSceneRenderer.lock()->GetGBufferAView();
		gbufferAInfos.push_back(texInfo);
	}
	creation.SetInputAttachment(0, gbufferAInfos);

	std::vector<icpTextureRenderResourceInfo> gbufferBInfos;
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		icpTextureRenderResourceInfo texInfo{};
		texInfo.m_texSampler = VK_NULL_HANDLE;
		texInfo.m_texImageView = m_pSceneRenderer.lock()->GetGBufferBView();
		gbufferBInfos.push_back(texInfo);
	}
	creation.SetInputAttachment(1, gbufferBInfos);

	std::vector<icpTextureRenderResourceInfo> gbufferCInfos;
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		icpTextureRenderResourceInfo texInfo{};
		texInfo.m_texSampler = VK_NULL_HANDLE;
		texInfo.m_texImageView = m_pSceneRenderer.lock()->GetGBufferCView();
		gbufferCInfos.push_back(texInfo);
	}
	creation.SetInputAttachment(2, gbufferCInfos);

	std::vector<icpTextureRenderResourceInfo> depthInfos;
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		icpTextureRenderResourceInfo texInfo{};
		texInfo.m_texSampler = VK_NULL_HANDLE;
		texInfo.m_texImageView = m_rhi->GetDepthImageView();
		depthInfos.push_back(texInfo);
	}
	creation.SetInputAttachment(3, depthInfos);

	m_rhi->CreateDescriptorSet(creation, m_vGBufferDSs);

	icpDescriptorSetCreation csmCreation{};
	csmCreation.layoutInfo = m_DSLayouts[eDeferredCompositePassDSType::CSM];

	std::vector<icpBufferRenderResourceInfo> bufferInfos;
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		icpBufferRenderResourceInfo bufferInfo{};
		bufferInfo.buffer = g_system_container.m_renderSystem->m_shadowManager->m_cascadeShadowMapCBs[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UBOCSM);
		bufferInfos.push_back(bufferInfo);
	}
	csmCreation.SetUniformBuffer(0, bufferInfos);

	FSamplerBuilderInfo buildInfo;
	buildInfo.BorderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	buildInfo.AddressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	buildInfo.RHI = m_rhi;
	auto csmSampler = icpSamplerBuilder::BuildSampler(buildInfo);

	auto csmPass = std::dynamic_pointer_cast<icpCSMPass>(m_pSceneRenderer.lock()->AccessRenderPass(eRenderPass::CSM_PASS));

	std::vector<icpTextureRenderResourceInfo> csmInfos;
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		icpTextureRenderResourceInfo texInfo{};
		texInfo.m_texSampler = csmSampler;
		texInfo.m_texImageView = csmPass->m_csmArrayViews[s_csmCascadeCount];
		csmInfos.push_back(texInfo);
	}
	csmCreation.SetCombinedImageSampler(1, csmInfos);

	m_rhi->CreateDescriptorSet(csmCreation, m_csmDSs);
}


INCEPTION_END_NAMESPACE