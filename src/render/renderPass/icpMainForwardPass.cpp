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
#include "../light/icpLightComponent.h"

INCEPTION_BEGIN_NAMESPACE
icpMainForwardPass::~icpMainForwardPass()
{
	
}

void icpMainForwardPass::initializeRenderPass(RendePassInitInfo initInfo)
{
	m_rhi = initInfo.rhi;

	createDescriptorSetLayouts();
	CreateSceneCB();
	allocateDescriptorSets();

	createRenderPass();
	setupPipeline();
	createFrameBuffers();
}

void icpMainForwardPass::createRenderPass()
{
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = m_rhi->m_swapChainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = icpVulkanUtility::findDepthFormat(m_rhi->m_physicalDevice);
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	std::array<VkAttachmentDescription, 2> attachments{ colorAttachment, depthAttachment };

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(m_rhi->m_device, &renderPassInfo, nullptr, &m_renderPassObj) != VK_SUCCESS) {
		throw std::runtime_error("failed to create render pass!");
	}
}

void icpMainForwardPass::setupPipeline()
{
	VkGraphicsPipelineCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

	// Shader Configuration
	VkPipelineShaderStageCreateInfo vertShader{};
	VkPipelineShaderStageCreateInfo fragShader{};

	vertShader.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	auto vertShaderPath = g_system_container.m_configSystem->m_shaderFolderPath / "verttest.spv";
	vertShader.module = icpVulkanUtility::createShaderModule(vertShaderPath.generic_string().c_str(), m_rhi->m_device);
	vertShader.stage = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;
	vertShader.pName = "main";

	fragShader.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	auto fragShaderPath = g_system_container.m_configSystem->m_shaderFolderPath / "fragmenttest.spv";
	fragShader.module = icpVulkanUtility::createShaderModule(fragShaderPath.generic_string().c_str(), m_rhi->m_device);
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
	inputAsm.topology = VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	info.pInputAssemblyState = &inputAsm;

	// Layout
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = m_DSLayouts.size();
	pipelineLayoutInfo.pSetLayouts = m_DSLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 0;

	if (vkCreatePipelineLayout(m_rhi->m_device, &pipelineLayoutInfo, nullptr, &m_pipelineInfo.m_pipelineLayout) != VK_SUCCESS)
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
	viewport.width = m_rhi->m_swapChainExtent.width;
	viewport.height = m_rhi->m_swapChainExtent.height;
	viewport.minDepth = 0.f;
	viewport.maxDepth = 1.f;

	viewportState.pViewports = &viewport;
	viewportState.viewportCount = 1;

	VkRect2D scissor;
	scissor.extent = m_rhi->m_swapChainExtent;
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
	info.renderPass = m_renderPassObj;
	info.subpass = 0;

	info.basePipelineHandle = VK_NULL_HANDLE;

	if (vkCreateGraphicsPipelines(m_rhi->m_device, VK_NULL_HANDLE, 1, &info, VK_NULL_HANDLE, &m_pipelineInfo.m_pipeline) != VK_SUCCESS)
	{
		throw std::runtime_error("create renderPipeline failed");
	}

	vkDestroyShaderModule(m_rhi->m_device, vertShader.module, nullptr);
	vkDestroyShaderModule(m_rhi->m_device, fragShader.module, nullptr);
}

void icpMainForwardPass::createFrameBuffers()
{
	m_swapChainFramebuffers.resize(m_rhi->m_swapChainImageViews.size());

	for (size_t i = 0; i < m_rhi->m_swapChainImageViews.size(); i++)
	{
		std::array<VkImageView, 2> attachments =
		{
			m_rhi->m_swapChainImageViews[i],
			m_rhi->m_depthImageView
		};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = m_renderPassObj;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = m_rhi->m_swapChainExtent.width;
		framebufferInfo.height = m_rhi->m_swapChainExtent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(m_rhi->m_device, &framebufferInfo, nullptr, &m_swapChainFramebuffers[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create framebuffer!");
		}
	}
}

void icpMainForwardPass::cleanup()
{
	cleanupSwapChain();

	vkDestroyRenderPass(m_rhi->m_device, m_renderPassObj, nullptr);
	vkDestroyPipelineLayout(m_rhi->m_device, m_pipelineInfo.m_pipelineLayout, nullptr);
	vkDestroyPipeline(m_rhi->m_device, m_pipelineInfo.m_pipeline, nullptr);
}

void icpMainForwardPass::cleanupSwapChain()
{
	for (auto framebuffer : m_swapChainFramebuffers)
	{
		vkDestroyFramebuffer(m_rhi->m_device, framebuffer, nullptr);
	}
}

void icpMainForwardPass::render(uint32_t frameBufferIndex, uint32_t currentFrame, VkResult acquireImageResult, VkSubmitInfo& info)
{
	if (acquireImageResult == VK_ERROR_OUT_OF_DATE_KHR)
	{
		recreateSwapChain();
		return;
	}
	else if (acquireImageResult != VK_SUCCESS && acquireImageResult != VK_SUBOPTIMAL_KHR)
	{
		throw std::runtime_error("failed to present swap chain image!");
	}

	updateGlobalBuffers(currentFrame);

	m_rhi->resetCommandBuffer(currentFrame);
	recordCommandBuffer(m_rhi->m_graphicsCommandBuffers[currentFrame], frameBufferIndex, currentFrame);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	m_waitSemaphores[0] = m_rhi->m_imageAvailableForRenderingSemaphores[currentFrame];
	m_waitStages[0] = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT ;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = m_waitSemaphores;
	submitInfo.pWaitDstStageMask = m_waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_rhi->m_graphicsCommandBuffers[currentFrame];

	info = submitInfo;
}

void icpMainForwardPass::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t curFrame)
{
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("failed to begin recording command buffer!");
	}

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = m_renderPassObj;
	renderPassInfo.framebuffer = m_swapChainFramebuffers[imageIndex];
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = m_rhi->m_swapChainExtent;

	std::array<VkClearValue, 2> clearColors{};
	clearColors[0].color = { {0.f,0.f,0.f,1.f} };
	clearColors[1].depthStencil = { 1.f, 0 };

	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearColors.size());
	renderPassInfo.pClearValues = clearColors.data();

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineInfo.m_pipeline);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)m_rhi->m_swapChainExtent.width;
	viewport.height = (float)m_rhi->m_swapChainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = m_rhi->m_swapChainExtent;
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	std::vector<VkDeviceSize> offsets{ 0 };

	vkCmdBindDescriptorSets(commandBuffer, VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineInfo.m_pipelineLayout, 0, 1, &m_perFrameDSs[curFrame], 0, nullptr);

	auto materialSubSystem = g_system_container.m_renderSystem->m_materialSystem;

	for(auto materialInstance : materialSubSystem->m_vMaterialContainer)
	{
		vkCmdBindDescriptorSets(commandBuffer, VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineInfo.m_pipelineLayout, 1, 3, &materialInstance->m_perMaterialDSs[curFrame], 0, nullptr);
	}

	std::vector<std::shared_ptr<icpGameEntity>> rootList;
	g_system_container.m_sceneSystem->getRootEntityList(rootList);

	for (auto entity: rootList)
	{
		if (entity->hasComponent<icpMeshRendererComponent>())
		{
			const auto& meshRender = entity->accessComponent<icpMeshRendererComponent>();
			auto& meshResId = meshRender.m_meshResId;
			auto res = g_system_container.m_resourceSystem->m_resources.m_allResources[icpResourceType::MESH][meshResId];
			auto meshRes = dynamic_pointer_cast<icpMeshResource>(res);

			auto vertBuf = meshRender.m_vertexBuffer;
			std::vector<VkBuffer>vertexBuffers{ vertBuf };

			vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers.data(), offsets.data());
			vkCmdBindIndexBuffer(commandBuffer, meshRender.m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);

			vkCmdBindDescriptorSets(commandBuffer, VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineInfo.m_pipelineLayout, 2, 1, &meshRender.m_perMeshDSs[curFrame], 0, nullptr);
			vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(meshRes->m_meshData.m_vertexIndices.size()), 1, 0, 0, 0);
		}
		/*
		else if (entity->hasComponent<icpPrimitiveRendererComponent>())
		{
			auto& primitive = entity->accessComponent<icpPrimitiveRendererComponent>();
			auto vertBuf = primitive.m_vertexBuffer;
			std::vector<VkBuffer>vertexBuffers{ vertBuf };

			vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers.data(), offsets.data());
			vkCmdBindIndexBuffer(commandBuffer, primitive.m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);

			auto descriptorSets = primitive.m_descriptorSets;
			vkCmdBindDescriptorSets(commandBuffer, VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineInfo.m_pipelineLayout, 0, 1, &descriptorSets[curFrame], 0, nullptr);
			vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(primitive.m_vertexIndices.size()), 1, 0, 0, 0);
		}
		*/
	}

	vkCmdEndRenderPass(commandBuffer);

	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to record command buffer!");
	}
}

void icpMainForwardPass::recreateSwapChain() {

	int width = 0, height = 0;
	glfwGetFramebufferSize(m_rhi->m_window, &width, &height);
	while (width == 0 || height == 0)
	{
		glfwGetFramebufferSize(m_rhi->m_window, &width, &height);
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(m_rhi->m_device);

	cleanupSwapChain();
	m_rhi->cleanupSwapChain();

	m_rhi->createSwapChain();
	m_rhi->createSwapChainImageViews();
	m_rhi->createDepthResources();
	createFrameBuffers();
}


void icpMainForwardPass::updateGlobalBuffers(uint32_t curFrame)
{
	auto camera = g_system_container.m_cameraSystem->getCurrentCamera();

	perFrameCB ssbo{};

	ssbo.view = g_system_container.m_cameraSystem->getCameraViewMatrix(camera);
	auto aspectRatio = (float)m_rhi->m_swapChainExtent.width / (float)m_rhi->m_swapChainExtent.height;
	ssbo.projection = glm::perspective(camera->m_fov, aspectRatio, camera->m_near, camera->m_far);
	ssbo.projection[1][1] *= -1;

	auto lightView = g_system_container.m_sceneSystem->m_registry.view<icpLightComponent>();

	int index = 0;
	for (auto& light : lightView)
	{
		auto& lightComp = lightView.get<icpLightComponent>(light);
		switch (lightComp.m_type)
		{
			case eLightType::DIRECTIONAL_LIGHT:
			{
				auto dirL = dynamic_cast<icpDirectionalLightComponent&>(lightComp);
				ssbo.dirLight.ambient = lightComp.m_ambient;
				ssbo.dirLight.diffuse = lightComp.m_diffuse;
				ssbo.dirLight.direction = dirL.m_direction;
				ssbo.dirLight.specular = lightComp.m_specular;
			}
			break;
			case eLightType::POINT_LIGHT:
			{
				auto pointLight = dynamic_cast<icpPointLightComponent&>(lightComp);
				PointLightRenderResource point{};
				point.ambient = pointLight.m_ambient;
				point.diffuse = pointLight.m_diffuse;
				point.specular = pointLight.m_specular;
				point.position = pointLight.m_position;
				point.constant = pointLight.constant;
				point.linear = pointLight.linear;
				point.quadratic = pointLight.quadratic;
				ssbo.pointLight[index] = point;
				index++;
			}
			break;
			default:
				break;
		}
	}

	if (!lightView.empty())
	{
		void* data;
		vmaMapMemory(m_rhi->m_vmaAllocator, m_perFrameCBAllocations[curFrame], &data);
		memcpy(data, &ssbo, sizeof(ssbo));
		vmaUnmapMemory(m_rhi->m_vmaAllocator, m_perFrameCBAllocations[curFrame]);
	}

	auto view = g_system_container.m_sceneSystem->m_registry.view<icpMeshRendererComponent, icpXFormComponent>();

	for (auto& entity: view)
	{
		auto& meshRenderer = view.get<icpMeshRendererComponent>(entity);

		auto& xformComp = view.get<icpXFormComponent>(entity);

		// for temp use todo: remove it
		auto firstRotate = glm::rotate(glm::mat4(1.f), glm::radians(-90.0f), glm::vec3(0.f, 0.f, 1.f));
		auto secondRotate = glm::rotate(glm::mat4(1.f), glm::radians(-90.0f), glm::vec3(1.f, 0.f, 0.f));

		UBOMeshRenderResource ubo{};
		ubo.model = secondRotate * firstRotate;

		void* data;
		vmaMapMemory(m_rhi->m_vmaAllocator, meshRenderer.m_perMeshUniformBufferAllocations[curFrame], &data);
		memcpy(data, &ubo, sizeof(ubo));
		vmaUnmapMemory(m_rhi->m_vmaAllocator, meshRenderer.m_perMeshUniformBufferAllocations[curFrame]);

		// todo classify different materialInstance
		for (auto& material : meshRenderer.m_materials)
		{
			void* pConst = material->MapUniformBuffer(curFrame);
			/*
			icpBlinnPhongMaterialInstance::UBOPerMaterial perMaterialCB{};

			perMaterialCB.shininess = 1.f;

			void* materialData;
			vmaMapMemory(m_rhi->m_vmaAllocator, material->m_perMaterialUniformBufferAllocations[curFrame], &data);
			memcpy(materialData, &perMaterialCB, sizeof(perMaterialCB));
			vmaUnmapMemory(m_rhi->m_vmaAllocator, material->m_perMaterialUniformBufferAllocations[curFrame]);
			*/
		}

	}

	/*
	auto primitiveView = g_system_container.m_sceneSystem->m_registry.view<icpPrimitiveRendererComponent, icpXFormComponent>();
	for (auto entity: primitiveView)
	{
		auto& primitiveRender = primitiveView.get<icpPrimitiveRendererComponent>(entity);
		ubo.model = glm::mat4(1.f);

		void* data;
		vkMapMemory(m_rhi->m_device, primitiveRender.m_uniformBufferMem[curFrame], 0, sizeof(ubo), 0, &data);
		memcpy(data, &ubo, sizeof(ubo));
		vkUnmapMemory(m_rhi->m_device, primitiveRender.m_uniformBufferMem[curFrame]);
	}
	*/
}


void icpMainForwardPass::createDescriptorSetLayouts()
{
	m_DSLayouts.resize(eMainForwardPassDSType::LAYOUT_TYPE_COUNT);
	// per mesh
	{
		// set 0, binding 0 
		VkDescriptorSetLayoutBinding perObjectSSBOBinding{};
		perObjectSSBOBinding.binding = 0;
		perObjectSSBOBinding.descriptorCount = 1;
		perObjectSSBOBinding.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		perObjectSSBOBinding.pImmutableSamplers = nullptr;
		perObjectSSBOBinding.stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;

		std::array<VkDescriptorSetLayoutBinding, 1> bindings{ perObjectSSBOBinding };

		VkDescriptorSetLayoutCreateInfo createInfo{};
		createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		createInfo.pBindings = bindings.data();
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

		if (vkCreateDescriptorSetLayout(m_rhi->m_device, &createInfo, nullptr, &m_DSLayouts[eMainForwardPassDSType::PER_MESH]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create descriptor set layout!");
		}
	}

	// per Material
	{
		VkDescriptorSetLayoutBinding perMaterialUBOBinding{};
		perMaterialUBOBinding.binding = 0;
		perMaterialUBOBinding.descriptorCount = 1;
		perMaterialUBOBinding.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		perMaterialUBOBinding.pImmutableSamplers = nullptr;
		perMaterialUBOBinding.stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;

		VkDescriptorSetLayoutBinding perMaterialDiffuseSamplerLayoutBinding{};
		perMaterialDiffuseSamplerLayoutBinding.binding = 1;
		perMaterialDiffuseSamplerLayoutBinding.descriptorCount = 1;
		perMaterialDiffuseSamplerLayoutBinding.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		perMaterialDiffuseSamplerLayoutBinding.pImmutableSamplers = nullptr;
		perMaterialDiffuseSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutBinding perMaterialSpecularSamplerLayoutBinding{};
		perMaterialSpecularSamplerLayoutBinding.binding = 2;
		perMaterialSpecularSamplerLayoutBinding.descriptorCount = 1;
		perMaterialSpecularSamplerLayoutBinding.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		perMaterialSpecularSamplerLayoutBinding.pImmutableSamplers = nullptr;
		perMaterialSpecularSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		std::array<VkDescriptorSetLayoutBinding, 3> bindings{ perMaterialUBOBinding, perMaterialDiffuseSamplerLayoutBinding, perMaterialSpecularSamplerLayoutBinding };

		VkDescriptorSetLayoutCreateInfo createInfo{};
		createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		createInfo.pBindings = bindings.data();
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

		if (vkCreateDescriptorSetLayout(m_rhi->m_device, &createInfo, nullptr, &m_DSLayouts[eMainForwardPassDSType::PER_MATERIAL]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create descriptor set layout!");
		}
	}

	// perFrame
	{
		VkDescriptorSetLayoutBinding perFrameSSBOBinding{};
		perFrameSSBOBinding.binding = 0;
		perFrameSSBOBinding.descriptorCount = 1;
		perFrameSSBOBinding.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		perFrameSSBOBinding.pImmutableSamplers = nullptr;
		perFrameSSBOBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		std::array<VkDescriptorSetLayoutBinding, 1> bindings{ perFrameSSBOBinding };

		VkDescriptorSetLayoutCreateInfo createInfo{};
		createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		createInfo.pBindings = bindings.data();
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

		if (vkCreateDescriptorSetLayout(m_rhi->m_device, &createInfo, nullptr, &m_DSLayouts[eMainForwardPassDSType::PER_FRAME]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create descriptor set layout!");
		}
	}
}

void icpMainForwardPass::CreateSceneCB()
{
	auto perFrameSize = sizeof(perFrameCB);
	VkSharingMode mode = m_rhi->m_queueIndices.m_graphicsFamily.value() == m_rhi->m_queueIndices.m_transferFamily.value() ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;

	m_perFrameCBs.resize(MAX_FRAMES_IN_FLIGHT);
	m_perFrameCBAllocations.resize(MAX_FRAMES_IN_FLIGHT);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		icpVulkanUtility::CreateGPUBuffer(
			perFrameSize,
			mode,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			m_rhi->m_vmaAllocator,
			m_perFrameCBAllocations[i],
			m_perFrameCBs[i]
		);
	}
}

void icpMainForwardPass::allocateDescriptorSets()
{
	std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, m_DSLayouts[eMainForwardPassDSType::PER_FRAME]);

	// per frame
	VkDescriptorSetAllocateInfo allocateInfo{};
	allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocateInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
	allocateInfo.descriptorPool = m_rhi->m_descriptorPool;
	allocateInfo.pSetLayouts = layouts.data();

	m_perFrameDSs.resize(MAX_FRAMES_IN_FLIGHT);

	if (vkAllocateDescriptorSets(m_rhi->m_device, &allocateInfo, m_perFrameDSs.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate descriptor sets!");
	}

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = m_perFrameCBs[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(perFrameCB);

		std::array<VkWriteDescriptorSet, 1> descriptorWrites{};
		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = m_perFrameDSs[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;

		vkUpdateDescriptorSets(m_rhi->m_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}


INCEPTION_END_NAMESPACE