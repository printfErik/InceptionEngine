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
#include "../RHI/icpDescirptorSet.h"
#include "../RHI/icpGPUBuffer.h"

INCEPTION_BEGIN_NAMESPACE

icpMainForwardPass::~icpMainForwardPass()
{
	
}

void icpMainForwardPass::initializeRenderPass(RenderPassInitInfo initInfo)
{
	m_rhi = initInfo.device;

	CreateDescriptorSetLayouts();
	CreateSceneCB();
	AllocateDescriptorSets();

	AllocateCommandBuffers();

	createRenderPass();
	setupPipeline();
	createFrameBuffers();
}

void icpMainForwardPass::createRenderPass()
{
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = m_rhi->GetSwapChainImageFormat();
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = icpVulkanUtility::findDepthFormat(m_rhi->GetPhysicalDevice());
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

	if (vkCreateRenderPass(m_rhi->GetLogicalDevice(), &renderPassInfo, nullptr, &m_renderPassObj) != VK_SUCCESS) {
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
	pipelineLayoutInfo.setLayoutCount = m_DSLayouts.size();

	std::vector<VkDescriptorSetLayout> layouts;
	for (auto& layoutInfo : m_DSLayouts)
	{
		layouts.push_back(layoutInfo.layout);
	}

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
	info.renderPass = m_renderPassObj;
	info.subpass = 0;

	info.basePipelineHandle = VK_NULL_HANDLE;

	if (vkCreateGraphicsPipelines(m_rhi->GetLogicalDevice(), VK_NULL_HANDLE, 1, &info, VK_NULL_HANDLE, &m_pipelineInfo.m_pipeline) != VK_SUCCESS)
	{
		throw std::runtime_error("create renderPipeline failed");
	}

	vkDestroyShaderModule(m_rhi->GetLogicalDevice(), vertShader.module, nullptr);
	vkDestroyShaderModule(m_rhi->GetLogicalDevice(), fragShader.module, nullptr);
}

void icpMainForwardPass::createFrameBuffers()
{
	auto& imageViews = m_rhi->GetSwapChainImageViews();
	m_swapChainFramebuffers.resize(imageViews.size());

	for (size_t i = 0; i < imageViews.size(); i++)
	{
		std::array<VkImageView, 2> attachments =
		{
			imageViews[i],
			m_rhi->GetDepthImageView()
		};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = m_renderPassObj;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = m_rhi->GetSwapChainExtent().width;
		framebufferInfo.height = m_rhi->GetSwapChainExtent().height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(m_rhi->GetLogicalDevice(), &framebufferInfo, nullptr, &m_swapChainFramebuffers[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create framebuffer!");
		}
	}
}

void icpMainForwardPass::cleanup()
{
	cleanupSwapChain();

	vkDestroyRenderPass(m_rhi->GetLogicalDevice(), m_renderPassObj, nullptr);
	vkDestroyPipelineLayout(m_rhi->GetLogicalDevice(), m_pipelineInfo.m_pipelineLayout, nullptr);
	vkDestroyPipeline(m_rhi->GetLogicalDevice(), m_pipelineInfo.m_pipeline, nullptr);
}

void icpMainForwardPass::cleanupSwapChain()
{
	for (auto framebuffer : m_swapChainFramebuffers)
	{
		vkDestroyFramebuffer(m_rhi->GetLogicalDevice(), framebuffer, nullptr);
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

	UpdateGlobalBuffers(currentFrame);

	vkResetCommandBuffer(m_commandBuffers[currentFrame], 0);
	recordCommandBuffer(m_commandBuffers[currentFrame], frameBufferIndex, currentFrame);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	auto semaphores = m_rhi->GetImageAvailableForRenderingSemaphores();
	m_waitSemaphores[0] = semaphores[currentFrame];
	m_waitStages[0] = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT ;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = m_waitSemaphores;
	submitInfo.pWaitDstStageMask = m_waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_commandBuffers[currentFrame];

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
	renderPassInfo.renderArea.extent = m_rhi->GetSwapChainExtent();

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

	vkCmdBindDescriptorSets(commandBuffer, VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineInfo.m_pipelineLayout, 2, 1, &m_perFrameDSs[curFrame], 0, nullptr);

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

	vkCmdEndRenderPass(commandBuffer);

	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to record command buffer!");
	}
}

void icpMainForwardPass::recreateSwapChain() {

	int width = 0, height = 0;
	glfwGetFramebufferSize(m_rhi->GetWindow(), &width, &height);
	while (width == 0 || height == 0)
	{
		glfwGetFramebufferSize(m_rhi->GetWindow(), &width, &height);
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(m_rhi->GetLogicalDevice());

	cleanupSwapChain();
	m_rhi->CleanUpSwapChain();

	m_rhi->CreateSwapChain();
	m_rhi->CreateSwapChainImageViews();
	m_rhi->CreateDepthResources();
	createFrameBuffers();
}


void icpMainForwardPass::UpdateGlobalBuffers(uint32_t curFrame)
{
	auto camera = g_system_container.m_cameraSystem->getCurrentCamera();

	perFrameCB CBPerFrame{};

	CBPerFrame.view = g_system_container.m_cameraSystem->getCameraViewMatrix(camera);
	auto aspectRatio = (float)m_rhi->GetSwapChainExtent().width / (float)m_rhi->GetSwapChainExtent().height;
	CBPerFrame.projection = glm::perspective(camera->m_fov, aspectRatio, camera->m_near, camera->m_far);
	CBPerFrame.projection[1][1] *= -1;

	CBPerFrame.camPos = camera->m_position;

	auto lightView = g_system_container.m_sceneSystem->m_registry.view<icpDirectionalLightComponent>();

	int index = 0;
	for (auto& light : lightView)
	{
		auto& lightComp = lightView.get<icpDirectionalLightComponent>(light);
		auto dirL = dynamic_cast<icpDirectionalLightComponent&>(lightComp);
		CBPerFrame.dirLight.color = glm::vec4(dirL.m_color,1.f);
		CBPerFrame.dirLight.direction = glm::vec4(dirL.m_direction, 0.f);

		/*
			case eLightType::POINT_LIGHT:
			{
				auto pointLight = dynamic_cast<icpPointLightComponent&>(lightComp);
				PointLightRenderResource point{};
				point.position = pointLight.m_position;
				point.constant = pointLight.constant;
				point.linear = pointLight.linear;
				point.quadratic = pointLight.quadratic;
				CBPerFrame.pointLight[index] = point;
				index++;
			}
			break;
			default:
				break;
		}
		*/
	}

	CBPerFrame.pointLightNumber = 0.f;

	if (!lightView.empty())
	{
		void* data;
		vmaMapMemory(m_rhi->GetVmaAllocator(), m_perFrameCBAllocations[curFrame], &data);
		memcpy(data, &CBPerFrame, sizeof(perFrameCB));
		vmaUnmapMemory(m_rhi->GetVmaAllocator(), m_perFrameCBAllocations[curFrame]);
	}

	
	auto view = g_system_container.m_sceneSystem->m_registry.view<icpMeshRendererComponent, icpXFormComponent>();

	for (auto& entity: view)
	{
		auto& meshRenderer = view.get<icpMeshRendererComponent>(entity);

		auto& xformComp = view.get<icpXFormComponent>(entity);

		UBOMeshRenderResource ubo{};
		ubo.model = xformComp.m_mtxTransform;
		ubo.normalMatrix = glm::transpose(glm::inverse(glm::mat3(ubo.model)));

		void* data;
		vmaMapMemory(m_rhi->GetVmaAllocator(), meshRenderer.m_perMeshUniformBufferAllocations[curFrame], &data);
		memcpy(data, &ubo, sizeof(UBOMeshRenderResource));
		vmaUnmapMemory(m_rhi->GetVmaAllocator(), meshRenderer.m_perMeshUniformBufferAllocations[curFrame]);

		// todo classify different materialInstance
		for (auto& material : meshRenderer.m_materials)
		{
			float fShininess = 1.f;
			void* materialData;
			vmaMapMemory(m_rhi->GetVmaAllocator(), material->m_perMaterialUniformBufferAllocations[curFrame], &materialData);
			memcpy(materialData, &fShininess, sizeof(float));
			vmaUnmapMemory(m_rhi->GetVmaAllocator(), material->m_perMaterialUniformBufferAllocations[curFrame]);
		}
	}
	
	auto primitiveView = g_system_container.m_sceneSystem->m_registry.view<icpPrimitiveRendererComponent, icpXFormComponent>();
	for (auto& primitive: primitiveView)
	{
		auto& primitiveRender = primitiveView.get<icpPrimitiveRendererComponent>(primitive);

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

		// todo classify different materialInstance
		for (auto& material : primitiveRender.m_vMaterials)
		{
			float fShininess = 1.f;
			void* materialData;
			vmaMapMemory(m_rhi->GetVmaAllocator(), material->m_perMaterialUniformBufferAllocations[curFrame], &materialData);
			memcpy(materialData, &fShininess, sizeof(float));
			vmaUnmapMemory(m_rhi->GetVmaAllocator(), material->m_perMaterialUniformBufferAllocations[curFrame]);
		}
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
		perMaterialUBOBinding.stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;
		m_DSLayouts[eMainForwardPassDSType::PER_MATERIAL].bindings.push_back({ VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER });

		// set 1, binding 1
		VkDescriptorSetLayoutBinding perMaterialDiffuseSamplerLayoutBinding{};
		perMaterialDiffuseSamplerLayoutBinding.binding = 1;
		perMaterialDiffuseSamplerLayoutBinding.descriptorCount = 1;
		perMaterialDiffuseSamplerLayoutBinding.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		perMaterialDiffuseSamplerLayoutBinding.pImmutableSamplers = nullptr;
		perMaterialDiffuseSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		m_DSLayouts[eMainForwardPassDSType::PER_MATERIAL].bindings.push_back({ VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER });

		// set 1, binding 2
		VkDescriptorSetLayoutBinding perMaterialSpecularSamplerLayoutBinding{};
		perMaterialSpecularSamplerLayoutBinding.binding = 2;
		perMaterialSpecularSamplerLayoutBinding.descriptorCount = 1;
		perMaterialSpecularSamplerLayoutBinding.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		perMaterialSpecularSamplerLayoutBinding.pImmutableSamplers = nullptr;
		perMaterialSpecularSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		m_DSLayouts[eMainForwardPassDSType::PER_MATERIAL].bindings.push_back({ VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER });

		// set 1, binding 3
		VkDescriptorSetLayoutBinding perMaterialNormalSamplerLayoutBinding{};
		perMaterialNormalSamplerLayoutBinding.binding = 3;
		perMaterialNormalSamplerLayoutBinding.descriptorCount = 1;
		perMaterialNormalSamplerLayoutBinding.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		perMaterialNormalSamplerLayoutBinding.pImmutableSamplers = nullptr;
		perMaterialNormalSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		m_DSLayouts[eMainForwardPassDSType::PER_MATERIAL].bindings.push_back({ VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER });

		// set 1, binding 4
		VkDescriptorSetLayoutBinding perMaterialRoughnessSamplerLayoutBinding{};
		perMaterialRoughnessSamplerLayoutBinding.binding = 4;
		perMaterialRoughnessSamplerLayoutBinding.descriptorCount = 1;
		perMaterialRoughnessSamplerLayoutBinding.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		perMaterialRoughnessSamplerLayoutBinding.pImmutableSamplers = nullptr;
		perMaterialRoughnessSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		m_DSLayouts[eMainForwardPassDSType::PER_MATERIAL].bindings.push_back({ VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER });
		
		std::array<VkDescriptorSetLayoutBinding, 5> bindings{ perMaterialUBOBinding, perMaterialSpecularSamplerLayoutBinding, perMaterialDiffuseSamplerLayoutBinding, perMaterialNormalSamplerLayoutBinding, perMaterialRoughnessSamplerLayoutBinding};

		VkDescriptorSetLayoutCreateInfo createInfo{};
		createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		createInfo.pBindings = bindings.data();
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

		if (vkCreateDescriptorSetLayout(logicDevice, &createInfo, nullptr, &m_DSLayouts[eMainForwardPassDSType::PER_MATERIAL].layout) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create descriptor set layout!");
		}
	}

	// perFrame
	{
		// set 2, binding 0 
		VkDescriptorSetLayoutBinding perFrameUBOBinding{};
		perFrameUBOBinding.binding = 0;
		perFrameUBOBinding.descriptorCount = 1;
		perFrameUBOBinding.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		perFrameUBOBinding.pImmutableSamplers = nullptr;
		perFrameUBOBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		m_DSLayouts[eMainForwardPassDSType::PER_FRAME].bindings.push_back({ VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER });

		std::array<VkDescriptorSetLayoutBinding, 1> bindings{ perFrameUBOBinding };

		VkDescriptorSetLayoutCreateInfo createInfo{};
		createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		createInfo.pBindings = bindings.data();
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

		if (vkCreateDescriptorSetLayout(logicDevice, &createInfo, nullptr, &m_DSLayouts[eMainForwardPassDSType::PER_FRAME].layout) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create descriptor set layout!");
		}
	}
}

void icpMainForwardPass::CreateSceneCB()
{
	auto perFrameSize = sizeof(perFrameCB);
	VkSharingMode mode = m_rhi->GetQueueFamilyIndices().m_graphicsFamily.value() == m_rhi->GetQueueFamilyIndices().m_transferFamily.value() ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;

	m_perFrameCBs.resize(MAX_FRAMES_IN_FLIGHT);
	m_perFrameCBAllocations.resize(MAX_FRAMES_IN_FLIGHT);

	auto allocator = m_rhi->GetVmaAllocator();

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		icpVulkanUtility::CreateGPUBuffer(
			perFrameSize,
			mode,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			allocator,
			m_perFrameCBAllocations[i],
			m_perFrameCBs[i]
		);
	}
}

void icpMainForwardPass::AllocateDescriptorSets()
{
	icpDescriptorSetCreation creation{};
	auto layout = m_DSLayouts[icpMainForwardPass::eMainForwardPassDSType::PER_FRAME];

	creation.layoutInfo = layout;

	std::vector<icpBufferRenderResourceInfo> bufferInfos;
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		icpBufferRenderResourceInfo bufferInfo{};
		bufferInfo.buffer = m_perFrameCBs[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(perFrameCB);
		bufferInfos.push_back(bufferInfo);
	}

	creation.SetUniformBuffer(0, bufferInfos);
	m_rhi->CreateDescriptorSet(creation, m_perFrameDSs);
}

void icpMainForwardPass::AllocateCommandBuffers()
{
	m_commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

	VkCommandBufferAllocateInfo gAllocInfo{};
	gAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	gAllocInfo.commandPool = m_rhi->GetGraphicsCommandPool();
	gAllocInfo.commandBufferCount = (uint32_t)m_commandBuffers.size();
	gAllocInfo.level = VkCommandBufferLevel::VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	if (vkAllocateCommandBuffers(m_rhi->GetLogicalDevice(), &gAllocInfo, m_commandBuffers.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate graphics command buffer!");
	}
}



INCEPTION_END_NAMESPACE