#include "icpDeferredRenderer.h"

#include "RHI/Vulkan/icpVulkanUtility.h"
#include "../ui/editorUI/icpEditorUI.h"
#include "renderPass/icpDeferredCompositePass.h"
#include "renderPass/icpEditorUiPass.h"
#include "renderPass/icpGBufferPass.h"

INCEPTION_BEGIN_NAMESPACE
icpDeferredRenderer::~icpDeferredRenderer()
{
	Cleanup();
}


bool icpDeferredRenderer::Initialize(std::shared_ptr<icpGPUDevice> vulkanRHI)
{
	m_pDevice = vulkanRHI;

	CreateSceneCB();
	CreateGlobalSceneDescriptorSetLayout();
	AllocateGlobalSceneDescriptorSets();
	AllocateCommandBuffers();

	CreateGBufferAttachments();
	CreateDeferredRenderPass();
	CreateDeferredFrameBuffer();

	icpRenderPassBase::RenderPassInitInfo gbufferPassCreateInfo;
	gbufferPassCreateInfo.device = m_pDevice;
	gbufferPassCreateInfo.passType = eRenderPass::GBUFFER_PASS;
	gbufferPassCreateInfo.sceneRenderer = shared_from_this();
	std::shared_ptr<icpRenderPassBase> gbufferPass = std::make_shared<icpGBufferPass>();
	gbufferPass->InitializeRenderPass(gbufferPassCreateInfo);

	m_renderPasses.push_back(gbufferPass);

	icpRenderPassBase::RenderPassInitInfo deferredCompositePassCreateInfo;
	deferredCompositePassCreateInfo.device = m_pDevice;
	deferredCompositePassCreateInfo.passType = eRenderPass::DEFERRED_COMPOSITION_PASS;
	deferredCompositePassCreateInfo.sceneRenderer = shared_from_this();
	std::shared_ptr<icpRenderPassBase> deferredCompositePass = std::make_shared<icpDeferredCompositePass>();
	deferredCompositePass->InitializeRenderPass(deferredCompositePassCreateInfo);

	m_renderPasses.push_back(deferredCompositePass);

	/*
	icpRenderPassBase::RenderPassInitInfo editorUIInfo;
	editorUIInfo.device = m_pDevice;
	editorUIInfo.passType = eRenderPass::EDITOR_UI_PASS;
	editorUIInfo.editorUi = std::make_shared<icpEditorUI>();
	editorUIInfo.sceneRenderer = shared_from_this();
	std::shared_ptr<icpRenderPassBase> editorUIPass = std::make_shared<icpEditorUiPass>();
	editorUIPass->InitializeRenderPass(editorUIInfo);

	m_renderPasses.push_back(editorUIPass);
	*/
	return true;
}

void icpDeferredRenderer::Cleanup()
{
	icpSceneRenderer::Cleanup();
}

VkRenderPass icpDeferredRenderer::GetGBufferRenderPass()
{
	return m_deferredRenderPass;
}

VkCommandBuffer icpDeferredRenderer::GetDeferredCommandBuffer(uint32_t curFrame)
{
	return m_vDeferredCommandBuffers[curFrame];
}

void icpDeferredRenderer::CreateGBufferAttachments()
{
	VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

	icpVulkanUtility::CreateGPUImage(
		(float)m_pDevice->GetSwapChainExtent().width,
		(float)m_pDevice->GetSwapChainExtent().height,
		1,
		1,
		VK_FORMAT_R16G16B16A16_SFLOAT,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
		m_pDevice->GetVmaAllocator(),
		m_gBufferA,
		m_gBufferAAllocation
	);

	m_gBufferAView = icpVulkanUtility::CreateGPUImageView(
		m_gBufferA,
		VK_IMAGE_VIEW_TYPE_2D,
		VK_FORMAT_R16G16B16A16_SFLOAT,
		aspectMask,
		1,
		1,
		m_pDevice->GetLogicalDevice()
	);

	icpVulkanUtility::CreateGPUImage(
		(float)m_pDevice->GetSwapChainExtent().width,
		(float)m_pDevice->GetSwapChainExtent().height,
		1,
		1,
		VK_FORMAT_R16G16B16A16_SFLOAT,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
		m_pDevice->GetVmaAllocator(),
		m_gBufferB,
		m_gBufferBAllocation
	);

	m_gBufferBView = icpVulkanUtility::CreateGPUImageView(
		m_gBufferB,
		VK_IMAGE_VIEW_TYPE_2D,
		VK_FORMAT_R16G16B16A16_SFLOAT,
		aspectMask,
		1,
		1,
		m_pDevice->GetLogicalDevice()
	);

	icpVulkanUtility::CreateGPUImage(
		(float)m_pDevice->GetSwapChainExtent().width,
		(float)m_pDevice->GetSwapChainExtent().height,
		1,
		1,
		VK_FORMAT_R16G16B16A16_SFLOAT,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
		m_pDevice->GetVmaAllocator(),
		m_gBufferC,
		m_gBufferCAllocation
	);

	m_gBufferCView = icpVulkanUtility::CreateGPUImageView(
		m_gBufferC,
		VK_IMAGE_VIEW_TYPE_2D,
		VK_FORMAT_R16G16B16A16_SFLOAT,
		aspectMask,
		1,
		1,
		m_pDevice->GetLogicalDevice()
	);
}

void icpDeferredRenderer::CreateDeferredRenderPass()
{
	std::array<VkAttachmentDescription, 5> attachments{};
	// Color attachment
	attachments[0].format = m_pDevice->GetSwapChainImageFormat();
	attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	// Deferred attachments
	// Position
	attachments[1].format = VK_FORMAT_R16G16B16A16_SFLOAT;
	attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[1].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	// Normals
	attachments[2].format = VK_FORMAT_R16G16B16A16_SFLOAT;
	attachments[2].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[2].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[2].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[2].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	// Albedo
	attachments[3].format = VK_FORMAT_R16G16B16A16_SFLOAT;
	attachments[3].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[3].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[3].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[3].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[3].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[3].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[3].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// Depth attachment
	attachments[4].format = icpVulkanUtility::findDepthFormat(m_pDevice->GetPhysicalDevice());
	attachments[4].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[4].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[4].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[4].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[4].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[4].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[4].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	// Two subpasses
	std::array<VkSubpassDescription, 2> subpassDescriptions{};

	// First subpass: Fill G-Buffer components
	// ----------------------------------------------------------------------------------------

	VkAttachmentReference colorReferences[3];
	colorReferences[0] = { 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
	colorReferences[1] = { 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
	colorReferences[2] = { 3, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
	VkAttachmentReference depthReference = { 4, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

	subpassDescriptions[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescriptions[0].colorAttachmentCount = 3;
	subpassDescriptions[0].pColorAttachments = colorReferences;
	subpassDescriptions[0].pDepthStencilAttachment = &depthReference;

	// Second subpass: Final composition (using G-Buffer components)
	// ----------------------------------------------------------------------------------------

	VkAttachmentReference colorReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

	VkAttachmentReference inputReferences[4];
	inputReferences[0] = { 1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
	inputReferences[1] = { 2, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
	inputReferences[2] = { 3, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
	inputReferences[3] = { 4, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

	subpassDescriptions[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescriptions[1].colorAttachmentCount = 1;
	subpassDescriptions[1].pColorAttachments = &colorReference;
	//subpassDescriptions[1].pDepthStencilAttachment = &depthReference;
	// Use the color attachments filled in the first pass as input attachments
	subpassDescriptions[1].inputAttachmentCount = 4;
	subpassDescriptions[1].pInputAttachments = inputReferences;

	/*
	// Third subpass: Forward transparency
	// ----------------------------------------------------------------------------------------
	colorReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

	inputReferences[0] = { 1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

	subpassDescriptions[2].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescriptions[2].colorAttachmentCount = 1;
	subpassDescriptions[2].pColorAttachments = &colorReference;
	subpassDescriptions[2].pDepthStencilAttachment = &depthReference;
	// Use the color/depth attachments filled in the first pass as input attachments
	subpassDescriptions[2].inputAttachmentCount = 1;
	subpassDescriptions[2].pInputAttachments = inputReferences;
	*/
	// Subpass dependencies for layout transitions
	std::array<VkSubpassDependency, 3> dependencies;

	/*
	// This makes sure that writes to the depth image are done before we try to write to it again
	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;;
	dependencies[0].srcAccessMask = 0;
	dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = 0;
	*/
	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].srcAccessMask = 0;
	dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	// This dependency transitions the input attachment from color attachment to input attachment read
	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = 1;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[2].srcSubpass = 1;
	dependencies[2].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[2].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[2].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[2].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[2].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[2].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	/*
	dependencies[3].srcSubpass = 2;
	dependencies[3].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[3].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[3].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[3].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[3].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[3].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
	*/
	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = static_cast<uint32_t>(subpassDescriptions.size());
	renderPassInfo.pSubpasses = subpassDescriptions.data();
	renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
	renderPassInfo.pDependencies = dependencies.data();

	vkCreateRenderPass(m_pDevice->GetLogicalDevice(), &renderPassInfo, nullptr, &m_deferredRenderPass);
}

void icpDeferredRenderer::CreateDeferredFrameBuffer()
{
	auto& imageViews = m_pDevice->GetSwapChainImageViews();
	m_vDeferredFrameBuffers.resize(imageViews.size());

	for (size_t i = 0; i < imageViews.size(); i++)
	{
		std::array<VkImageView, 5> attachments =
		{
			imageViews[i],
			m_gBufferAView,
			m_gBufferBView,
			m_gBufferCView,
			m_pDevice->GetDepthImageView()
		};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = m_deferredRenderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = m_pDevice->GetSwapChainExtent().width;
		framebufferInfo.height = m_pDevice->GetSwapChainExtent().height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(m_pDevice->GetLogicalDevice(), &framebufferInfo, nullptr, &m_vDeferredFrameBuffers[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create framebuffer!");
		}
	}
}

void icpDeferredRenderer::Render()
{
	m_pDevice->WaitForFence(m_currentFrame);

	VkResult result;
	auto index = m_pDevice->AcquireNextImageFromSwapchain(m_currentFrame, result);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		RecreateSwapChain();
		return;
	}

	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		throw std::runtime_error("failed to present swap chain image!");
	}

	UpdateGlobalSceneCB(m_currentFrame);

	for (const auto renderPass : m_renderPasses)
	{
		renderPass->UpdateRenderPassCB(m_currentFrame);
	}

	ResetThenBeginCommandBuffer();
	BeginForwardRenderPass(index);

	for (const auto renderPass : m_renderPasses)
	{
		renderPass->Render(index, m_currentFrame, result);
	}

	EndForwardRenderPass();
	EndRecordingCommandBuffer();

	SubmitCommandList();

	Present(index);

	m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}


void icpDeferredRenderer::RecreateSwapChain()
{
	int width = 0, height = 0;
	glfwGetFramebufferSize(m_pDevice->GetWindow(), &width, &height);
	while (width == 0 || height == 0)
	{
		glfwGetFramebufferSize(m_pDevice->GetWindow(), &width, &height);
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(m_pDevice->GetLogicalDevice());

	CleanupSwapChain();
	m_pDevice->CleanUpSwapChain();

	m_pDevice->CreateSwapChain();
	m_pDevice->CreateSwapChainImageViews();
	m_pDevice->CreateDepthResources();

	CreateGBufferAttachments();
	CreateDeferredFrameBuffer();
}

void icpDeferredRenderer::CleanupSwapChain()
{
	for (auto framebuffer : m_vDeferredFrameBuffers)
	{
		vkDestroyFramebuffer(m_pDevice->GetLogicalDevice(), framebuffer, nullptr);
	}

	vmaDestroyImage(m_pDevice->GetVmaAllocator(), m_gBufferA, m_gBufferAAllocation);
	vmaDestroyImage(m_pDevice->GetVmaAllocator(), m_gBufferB, m_gBufferBAllocation);
	vmaDestroyImage(m_pDevice->GetVmaAllocator(), m_gBufferC, m_gBufferCAllocation);

	vkDestroyImageView(m_pDevice->GetLogicalDevice(), m_gBufferAView, nullptr);
	vkDestroyImageView(m_pDevice->GetLogicalDevice(), m_gBufferBView, nullptr);
	vkDestroyImageView(m_pDevice->GetLogicalDevice(), m_gBufferCView, nullptr);
}

void icpDeferredRenderer::AllocateCommandBuffers()
{
	m_vDeferredCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

	VkCommandBufferAllocateInfo gAllocInfo{};
	gAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	gAllocInfo.commandPool = m_pDevice->GetGraphicsCommandPool();
	gAllocInfo.commandBufferCount = (uint32_t)m_vDeferredCommandBuffers.size();
	gAllocInfo.level = VkCommandBufferLevel::VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	if (vkAllocateCommandBuffers(m_pDevice->GetLogicalDevice(), &gAllocInfo, m_vDeferredCommandBuffers.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate graphics command buffer!");
	}
}

VkImageView icpDeferredRenderer::GetGBufferAView()
{
	return m_gBufferAView;
}

VkImageView icpDeferredRenderer::GetGBufferBView()
{
	return m_gBufferBView;
}

VkImageView icpDeferredRenderer::GetGBufferCView()
{
	return m_gBufferCView;
}


void icpDeferredRenderer::ResetThenBeginCommandBuffer()
{
	vkResetCommandBuffer(m_vDeferredCommandBuffers[m_currentFrame], 0);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	if (vkBeginCommandBuffer(m_vDeferredCommandBuffers[m_currentFrame], &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("failed to begin recording command buffer!");
	}
}

void icpDeferredRenderer::BeginForwardRenderPass(uint32_t imageIndex)
{
	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = m_deferredRenderPass;
	renderPassInfo.framebuffer = m_vDeferredFrameBuffers[imageIndex];
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = m_pDevice->GetSwapChainExtent();

	std::array<VkClearValue, 5> clearColors{};
	clearColors[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
	clearColors[1].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
	clearColors[2].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
	clearColors[3].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
	clearColors[4].depthStencil = { 1.f, 0 };

	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearColors.size());
	renderPassInfo.pClearValues = clearColors.data();

	vkCmdBeginRenderPass(m_vDeferredCommandBuffers[m_currentFrame], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}


void icpDeferredRenderer::EndForwardRenderPass()
{
	vkCmdEndRenderPass(m_vDeferredCommandBuffers[m_currentFrame]);
}


void icpDeferredRenderer::EndRecordingCommandBuffer()
{
	if (vkEndCommandBuffer(m_vDeferredCommandBuffers[m_currentFrame]) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to record command buffer!");
	}
}

void icpDeferredRenderer::SubmitCommandList()
{
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	auto& semaphores = m_pDevice->GetImageAvailableForRenderingSemaphores();
	auto waitSemaphore = semaphores[m_currentFrame];

	VkPipelineStageFlags waitStage = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &waitSemaphore;
	submitInfo.pWaitDstStageMask = &waitStage;

	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &m_pDevice->GetRenderFinishedForPresentationSemaphores()[m_currentFrame];

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_vDeferredCommandBuffers[m_currentFrame];

	auto& fences = m_pDevice->GetInFlightFences();

	vkQueueSubmit(m_pDevice->GetGraphicsQueue(), 1, &submitInfo, fences[m_currentFrame]);
}

void icpDeferredRenderer::Present(uint32_t imageIndex)
{
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;

	auto& RenderFinishedForPresentationSemaphores = m_pDevice->GetRenderFinishedForPresentationSemaphores();
	presentInfo.pWaitSemaphores = &RenderFinishedForPresentationSemaphores[m_currentFrame];

	VkSwapchainKHR swapChains[] = { m_pDevice->GetSwapChain() };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;

	uint32_t _imageIndex = imageIndex;
	presentInfo.pImageIndices = &_imageIndex;

	VkResult result = vkQueuePresentKHR(m_pDevice->GetPresentQueue(), &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_pDevice->m_framebufferResized)
	{
		m_pDevice->m_framebufferResized = false;
		RecreateSwapChain();
	}
	else if (result != VK_SUCCESS)
	{
		throw std::runtime_error("failed to present swap chain image!");
	}
}

INCEPTION_END_NAMESPACE
