#include "icpMainForwardPass.h"
#include "../RHI/Vulkan/icpVulkanUtility.h"
#include "../../core/icpSystemContainer.h"
#include "../../core/icpConfigSystem.h"
#include "../../resource/icpResourceSystem.h"
#include "../../resource/icpResourceBase.h"
#include "../../mesh/icpMeshResource.h"
#include "../../mesh/icpMeshRendererComponent.h"
#include "../../scene/icpEntity.h"
#include "../../scene/icpSceneSystem.h"
#include "../icpCameraSystem.h"
#include "../../scene/icpXFormComponent.h"
#include "../icpRenderSystem.h"
#include "../../mesh/icpPrimitiveRendererComponent.h"
#include "../RHI/icpDescirptorSet.h"

INCEPTION_BEGIN_NAMESPACE

static constexpr uint32_t PBR_MATERIAL_TEXTURE_NUMBER = 7;

icpMainForwardPass::~icpMainForwardPass()
{
	
}

void icpMainForwardPass::InitializeRenderPass(RenderPassInitInfo initInfo)
{
	m_rhi = initInfo.device;
	m_renderPassMgr = initInfo.renderPassMgr;

	CreateDescriptorSetLayouts();
	SetupPipeline();
}

void icpMainForwardPass::SetupPipeline()
{
	auto renderPassMgr = m_renderPassMgr.lock();

	VkGraphicsPipelineCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

	// Shader Configuration
	VkPipelineShaderStageCreateInfo vertShader{};
	VkPipelineShaderStageCreateInfo fragShader{};

	vertShader.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	auto vertShaderPath = g_system_container.m_configSystem->m_shaderFolderPath / "VertPBR.spv";
	vertShader.module = icpVulkanUtility::createShaderModule(vertShaderPath.generic_string().c_str(), m_rhi->GetLogicalDevice());
	vertShader.stage = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;
	vertShader.pName = "main";

	fragShader.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	auto fragShaderPath = g_system_container.m_configSystem->m_shaderFolderPath / "FragPBR.spv";
	fragShader.module = icpVulkanUtility::createShaderModule(fragShaderPath.generic_string().c_str(), m_rhi->GetLogicalDevice());
	fragShader.stage = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShader.pName = "main";

	std::array<VkPipelineShaderStageCreateInfo, 2> shaderCreateInfos{
		vertShader, fragShader
	};

	info.stageCount = 2;
	info.pStages = shaderCreateInfos.data();

	// Vertex Input
	VkPipelineVertexInputStateCreateInfo vertexInput{};

	auto bindingDescription = icpVertex::getBindingDescription();
	auto attributeDescription = icpVertex::getAttributeDescription();

	vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInput.vertexBindingDescriptionCount = 1;
	vertexInput.pVertexBindingDescriptions = &bindingDescription;
	vertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescription.size());
	vertexInput.pVertexAttributeDescriptions = attributeDescription.data();

	info.pVertexInputState = &vertexInput;

	// Input Assembly
	VkPipelineInputAssemblyStateCreateInfo inputAsm{};
	inputAsm.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAsm.primitiveRestartEnable = VK_FALSE;
	inputAsm.topology = VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

	info.pInputAssemblyState = &inputAsm;

	// Layout
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	

	std::vector<VkDescriptorSetLayout> layouts{};
	for (auto& layoutInfo : m_DSLayouts)
	{
		layouts.push_back(layoutInfo.layout);
	}

	layouts.push_back(renderPassMgr->m_sceneDSLayout.layout);
	pipelineLayoutInfo.setLayoutCount = layouts.size(); // global scene ds
	pipelineLayoutInfo.pSetLayouts = layouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 0;

	if (vkCreatePipelineLayout(m_rhi->GetLogicalDevice(), &pipelineLayoutInfo, nullptr, &m_pipelineInfo.m_pipelineLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create pipeline layout!");
	}

	info.layout = m_pipelineInfo.m_pipelineLayout;

	// Viewport and Scissor
	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;

	VkViewport viewport;
	viewport.x = 0.f;
	viewport.y = 0.f;
	viewport.width = m_rhi->GetSwapChainExtent().width;
	viewport.height = m_rhi->GetSwapChainExtent().height;
	viewport.minDepth = 0.f;
	viewport.maxDepth = 1.f;

	viewportState.pViewports = &viewport;
	viewportState.viewportCount = 1;

	VkRect2D scissor;
	scissor.extent = m_rhi->GetSwapChainExtent();
	scissor.offset = { 0,0 };

	viewportState.pScissors = &scissor;
	viewportState.scissorCount = 1;

	info.pViewportState = &viewportState;

	// Rsterization State
	VkPipelineRasterizationStateCreateInfo rastInfo{};
	rastInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rastInfo.rasterizerDiscardEnable = VK_FALSE;
	rastInfo.depthClampEnable = VK_FALSE;
	rastInfo.depthBiasEnable = VK_FALSE;
	rastInfo.lineWidth = 1.f;
	rastInfo.cullMode = VkCullModeFlagBits::VK_CULL_MODE_BACK_BIT;
	rastInfo.frontFace = VkFrontFace::VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rastInfo.polygonMode = VkPolygonMode::VK_POLYGON_MODE_FILL;

	info.pRasterizationState = &rastInfo;

	// MultiSample
	VkPipelineMultisampleStateCreateInfo multiSampleState{};
	multiSampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multiSampleState.sampleShadingEnable = VK_FALSE;
	multiSampleState.rasterizationSamples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;

	info.pMultisampleState = &multiSampleState;

	// Depth and Stencil
	VkPipelineDepthStencilStateCreateInfo depthStencilState{};
	depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilState.depthTestEnable = VK_TRUE;
	depthStencilState.depthWriteEnable = VK_TRUE;
	depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencilState.depthBoundsTestEnable = VK_FALSE;
	depthStencilState.stencilTestEnable = VK_FALSE;

	info.pDepthStencilState = &depthStencilState;

	// Color Blend
	VkPipelineColorBlendStateCreateInfo colorBlend{};
	colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlend.logicOpEnable = VK_FALSE;
	colorBlend.logicOp = VK_LOGIC_OP_COPY;
	colorBlend.attachmentCount = 1;
	colorBlend.blendConstants[0] = 0.0f;
	colorBlend.blendConstants[1] = 0.0f;
	colorBlend.blendConstants[2] = 0.0f;
	colorBlend.blendConstants[3] = 0.0f;

	VkPipelineColorBlendAttachmentState attBlendState{};
	attBlendState.colorWriteMask = VkColorComponentFlagBits::VK_COLOR_COMPONENT_R_BIT
		| VkColorComponentFlagBits::VK_COLOR_COMPONENT_G_BIT
		| VkColorComponentFlagBits::VK_COLOR_COMPONENT_B_BIT
		| VkColorComponentFlagBits::VK_COLOR_COMPONENT_A_BIT;
	attBlendState.blendEnable = VK_FALSE;

	colorBlend.pAttachments = &attBlendState;

	info.pColorBlendState = &colorBlend;

	// Dynamic State
	std::vector<VkDynamicState> dynamicStates =
	{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
		VK_DYNAMIC_STATE_LINE_WIDTH
	};

	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	info.pDynamicState = &dynamicState;

	// RenderPass
	info.renderPass = renderPassMgr->m_mainForwardRenderPass;
	info.subpass = 0;

	info.basePipelineHandle = VK_NULL_HANDLE;

	if (vkCreateGraphicsPipelines(m_rhi->GetLogicalDevice(), VK_NULL_HANDLE, 1, &info, VK_NULL_HANDLE, &m_pipelineInfo.m_pipeline) != VK_SUCCESS)
	{
		throw std::runtime_error("create renderPipeline failed");
	}

	vkDestroyShaderModule(m_rhi->GetLogicalDevice(), vertShader.module, nullptr);
	vkDestroyShaderModule(m_rhi->GetLogicalDevice(), fragShader.module, nullptr);
}

void icpMainForwardPass::Cleanup()
{
	vkDestroyPipelineLayout(m_rhi->GetLogicalDevice(), m_pipelineInfo.m_pipelineLayout, nullptr);
	vkDestroyPipeline(m_rhi->GetLogicalDevice(), m_pipelineInfo.m_pipeline, nullptr);
}

void icpMainForwardPass::Render(uint32_t frameBufferIndex, uint32_t currentFrame, VkResult acquireImageResult)
{
	auto mgr = m_renderPassMgr.lock();
	//vkResetCommandBuffer(mgr->m_vMainForwardCommandBuffers[currentFrame], 0);
	RecordCommandBuffer(mgr->m_vMainForwardCommandBuffers[currentFrame], frameBufferIndex, currentFrame);
}

void icpMainForwardPass::RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t curFrame)
{
	auto mgr = m_renderPassMgr.lock();

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

	vkCmdBindDescriptorSets(commandBuffer, VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineInfo.m_pipelineLayout, 2, 1, &mgr->m_vSceneDSs[curFrame], 0, nullptr);

	auto materialSubSystem = g_system_container.m_renderSystem->GetMaterialSubSystem();

	for(auto materialInstance : materialSubSystem->m_vMaterialContainer)
	{
		vkCmdBindDescriptorSets(commandBuffer, VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineInfo.m_pipelineLayout, 1, 1, &(materialInstance->m_perMaterialDSs[curFrame]), 0, nullptr);
	}

	std::vector<std::shared_ptr<icpGameEntity>> rootList;
	g_system_container.m_sceneSystem->getRootEntityList(rootList);

	for (auto entity: rootList)
	{
		if (entity->hasComponent<icpMeshRendererComponent>())
		{
			const auto& meshRender = entity->accessComponent<icpMeshRendererComponent>();
			auto& meshResId = meshRender.m_meshResId;
			auto res = g_system_container.m_resourceSystem->GetResourceContainer()[icpResourceType::MESH][meshResId];
			auto meshRes = std::dynamic_pointer_cast<icpMeshResource>(res);

			auto vertBuf = meshRender.m_vertexBuffer;
			std::vector<VkBuffer>vertexBuffers{ vertBuf };

			vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers.data(), offsets.data());
			vkCmdBindIndexBuffer(commandBuffer, meshRender.m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);

			vkCmdBindDescriptorSets(commandBuffer, VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineInfo.m_pipelineLayout, 0, 1, &meshRender.m_perMeshDSs[curFrame], 0, nullptr);
			vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(meshRes->m_meshData.m_vertexIndices.size()), 1, 0, 0, 0);
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
			vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(primitive.m_vertexIndices.size()), 1, 0, 0, 0);
		}
	}
}

void icpMainForwardPass::UpdateRenderPassCB(uint32_t curFrame)
{
	auto view = g_system_container.m_sceneSystem->m_registry.view<icpMeshRendererComponent, icpXFormComponent>();

	for (auto& entity: view)
	{
		auto& meshRenderer = view.get<icpMeshRendererComponent>(entity);

		if (meshRenderer.m_pMaterial->m_shadingModel != eMaterialShadingModel::PBR_LIT)
		{
			continue;
		}

		auto& xformComp = view.get<icpXFormComponent>(entity);

		UBOMeshRenderResource ubo{};
		ubo.model = xformComp.m_mtxTransform;
		ubo.normalMatrix = glm::transpose(glm::inverse(glm::mat3(ubo.model)));

		void* data;
		vmaMapMemory(m_rhi->GetVmaAllocator(), meshRenderer.m_perMeshUniformBufferAllocations[curFrame], &data);
		memcpy(data, &ubo, sizeof(UBOMeshRenderResource));
		vmaUnmapMemory(m_rhi->GetVmaAllocator(), meshRenderer.m_perMeshUniformBufferAllocations[curFrame]);

		auto material = meshRenderer.m_pMaterial;

		void* materialData;
		vmaMapMemory(m_rhi->GetVmaAllocator(), material->m_perMaterialUniformBufferAllocations[curFrame], &materialData);
		memcpy(materialData, material->CheckMaterialDataCache(), sizeof(PBRShaderMaterial));
		vmaUnmapMemory(m_rhi->GetVmaAllocator(), material->m_perMaterialUniformBufferAllocations[curFrame]);

	}
	
	auto primitiveView = g_system_container.m_sceneSystem->m_registry.view<icpPrimitiveRendererComponent, icpXFormComponent>();
	for (auto& primitive: primitiveView)
	{
		auto& primitiveRender = primitiveView.get<icpPrimitiveRendererComponent>(primitive);

		if (primitiveRender.m_pMaterial->m_shadingModel != eMaterialShadingModel::PBR_LIT)
		{
			continue;
		}

		auto& xfom = primitiveRender.m_possessor->accessComponent<icpXFormComponent>();

		UBOMeshRenderResource ubo{};
		ubo.model = glm::mat4(1.f);

		ubo.model = glm::translate(ubo.model, xfom.m_translation);
		ubo.model = glm::scale(ubo.model, xfom.m_scale);
		auto mat = glm::mat3(ubo.model);
		ubo.normalMatrix = glm::transpose(glm::inverse(glm::mat3(ubo.model)));

		void* data;
		vmaMapMemory(m_rhi->GetVmaAllocator(), primitiveRender.m_uniformBufferAllocations[curFrame], &data);
		memcpy(data, &ubo, sizeof(UBOMeshRenderResource));
		vmaUnmapMemory(m_rhi->GetVmaAllocator(), primitiveRender.m_uniformBufferAllocations[curFrame]);

		auto material = primitiveRender.m_pMaterial;

		void* materialData;
		vmaMapMemory(m_rhi->GetVmaAllocator(), material->m_perMaterialUniformBufferAllocations[curFrame], &materialData);
		memcpy(materialData, material->CheckMaterialDataCache(), sizeof(PBRShaderMaterial));
		vmaUnmapMemory(m_rhi->GetVmaAllocator(), material->m_perMaterialUniformBufferAllocations[curFrame]);
	}
}

void icpMainForwardPass::CreateDescriptorSetLayouts()
{
	m_DSLayouts.resize(eMainForwardPassDSType::LAYOUT_TYPE_COUNT);
	auto logicDevice = m_rhi->GetLogicalDevice();
	// per mesh
	{
		// set 0, binding 0 
		VkDescriptorSetLayoutBinding perObjectSSBOBinding{};
		perObjectSSBOBinding.binding = 0;
		perObjectSSBOBinding.descriptorCount = 1;
		perObjectSSBOBinding.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		perObjectSSBOBinding.pImmutableSamplers = nullptr;
		perObjectSSBOBinding.stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;
		m_DSLayouts[eMainForwardPassDSType::PER_MESH].bindings.push_back({ VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER });

		std::array<VkDescriptorSetLayoutBinding, 1> bindings{ perObjectSSBOBinding };

		VkDescriptorSetLayoutCreateInfo createInfo{};
		createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		createInfo.pBindings = bindings.data();
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

		if (vkCreateDescriptorSetLayout(logicDevice, &createInfo, nullptr, &m_DSLayouts[eMainForwardPassDSType::PER_MESH].layout) != VK_SUCCESS)
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
		m_DSLayouts[eMainForwardPassDSType::PER_MATERIAL].bindings.push_back({ VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER });

		std::vector<VkDescriptorSetLayoutBinding> bindings {perMaterialUBOBinding};

		for (int i = 0; i < PBR_MATERIAL_TEXTURE_NUMBER; i++)
		{
			// set 1, binding i
			VkDescriptorSetLayoutBinding perMaterialTextureSamplerLayoutBinding{};
			perMaterialTextureSamplerLayoutBinding.binding = i + 1;
			perMaterialTextureSamplerLayoutBinding.descriptorCount = 1;
			perMaterialTextureSamplerLayoutBinding.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			perMaterialTextureSamplerLayoutBinding.pImmutableSamplers = nullptr;
			perMaterialTextureSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			m_DSLayouts[eMainForwardPassDSType::PER_MATERIAL].bindings.push_back({ VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER });

			bindings.push_back(perMaterialTextureSamplerLayoutBinding);
		}

		VkDescriptorSetLayoutCreateInfo createInfo{};
		createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		createInfo.pBindings = bindings.data();
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

		if (vkCreateDescriptorSetLayout(logicDevice, &createInfo, nullptr, &m_DSLayouts[eMainForwardPassDSType::PER_MATERIAL].layout) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create descriptor set layout!");
		}
	}
}



INCEPTION_END_NAMESPACE