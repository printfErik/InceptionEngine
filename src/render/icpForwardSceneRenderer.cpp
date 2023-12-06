#include "icpForwardSceneRenderer.h"
#include "RHI/Vulkan/icpVkGPUDevice.h"
#include "RHI/Vulkan/icpVulkanUtility.h"
#include "renderPass/icpMainForwardPass.h"
#include "renderPass/icpEditorUiPass.h"

#include <vulkan/vulkan.hpp>

#include "../ui/editorUI/icpEditorUI.h"
#include "renderPass/icpRenderPassBase.h"
#include "icpCameraSystem.h"
#include "../scene/icpSceneSystem.h"
#include "light/icpLightComponent.h"
#include "renderPass/icpUnlitForwardPass.h"

INCEPTION_BEGIN_NAMESPACE
icpForwardSceneRenderer::icpForwardSceneRenderer()
{
}


icpForwardSceneRenderer::~icpForwardSceneRenderer()
{
	Cleanup();
}

bool icpForwardSceneRenderer::Initialize(std::shared_ptr<icpGPUDevice> vulkanRHI)
{
	m_pDevice = vulkanRHI;

	CreateSceneCB();
	CreateGlobalSceneDescriptorSetLayout();
	AllocateGlobalSceneDescriptorSets();
	AllocateCommandBuffers();
	CreateForwardRenderPass();
	CreateSwapChainFrameBuffers();

	icpRenderPassBase::RenderPassInitInfo mainPassCreateInfo;
	mainPassCreateInfo.device = m_pDevice;
	mainPassCreateInfo.passType = eRenderPass::MAIN_FORWARD_PASS;
	mainPassCreateInfo.sceneRenderer = shared_from_this();
	std::shared_ptr<icpRenderPassBase> mainForwordPass = std::make_shared<icpMainForwardPass>();
	mainForwordPass->InitializeRenderPass(mainPassCreateInfo);

	m_renderPasses.push_back(mainForwordPass);

	icpRenderPassBase::RenderPassInitInfo unlitPassInfo;
	unlitPassInfo.device = m_pDevice;
	unlitPassInfo.passType = eRenderPass::UNLIT_PASS;
	unlitPassInfo.sceneRenderer = shared_from_this();
	std::shared_ptr<icpRenderPassBase> unlitPass = std::make_shared<icpUnlitForwardPass>();
	unlitPass->InitializeRenderPass(unlitPassInfo);

	m_renderPasses.push_back(unlitPass);
	

	icpRenderPassBase::RenderPassInitInfo editorUIInfo;
	editorUIInfo.device = m_pDevice;
	editorUIInfo.passType = eRenderPass::EDITOR_UI_PASS;
	editorUIInfo.editorUi = std::make_shared<icpEditorUI>();
	editorUIInfo.sceneRenderer = shared_from_this();
	std::shared_ptr<icpRenderPassBase> editorUIPass = std::make_shared<icpEditorUiPass>();
	editorUIPass->InitializeRenderPass(editorUIInfo);

	m_renderPasses.push_back(editorUIPass);

	return true;
}

void icpForwardSceneRenderer::Cleanup()
{
	icpSceneRenderer::Cleanup();
}

VkCommandBuffer icpForwardSceneRenderer::GetMainForwardCommandBuffer(uint32_t curFrame)
{
	return m_vMainForwardCommandBuffers[curFrame];
}

VkRenderPass icpForwardSceneRenderer::GetMainForwardRenderPass()
{
	return m_mainForwardRenderPass;
}

VkDescriptorSet icpForwardSceneRenderer::GetSceneDescriptorSet(uint32_t curFrame)
{
	return m_vSceneDSs[curFrame];
}


icpDescriptorSetLayoutInfo& icpForwardSceneRenderer::GetSceneDSLayout()
{
	return m_sceneDSLayout;
}


void icpForwardSceneRenderer::Render()
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

	for(const auto renderPass : m_renderPasses)
	{
		renderPass->Render(index, m_currentFrame, result);
	}

	EndForwardRenderPass();
	EndRecordingCommandBuffer();

	SubmitCommandList();

	Present(index);

	m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void icpForwardSceneRenderer::SubmitCommandList()
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
	submitInfo.pCommandBuffers = &m_vMainForwardCommandBuffers[m_currentFrame];

	auto& fences = m_pDevice->GetInFlightFences();

	vkQueueSubmit(m_pDevice->GetGraphicsQueue(), 1, &submitInfo, fences[m_currentFrame]);
}

void icpForwardSceneRenderer::Present(uint32_t imageIndex)
{
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;

	auto& RenderFinishedForPresentationSemaphores = m_pDevice->GetRenderFinishedForPresentationSemaphores();
	presentInfo.pWaitSemaphores = &RenderFinishedForPresentationSemaphores[m_currentFrame];

	VkSwapchainKHR swapChains[] = { m_pDevice->GetSwapChain() };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;

	presentInfo.pImageIndices = &imageIndex;

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

void icpForwardSceneRenderer::CreateForwardRenderPass()
{
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = m_pDevice->GetSwapChainImageFormat();
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = icpVulkanUtility::findDepthFormat(m_pDevice->GetPhysicalDevice());
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

	if (vkCreateRenderPass(m_pDevice->GetLogicalDevice(), &renderPassInfo, nullptr, &m_mainForwardRenderPass) != VK_SUCCESS) {
		throw std::runtime_error("failed to create render pass!");
	}
}

void icpForwardSceneRenderer::CreateSwapChainFrameBuffers()
{
	auto& imageViews = m_pDevice->GetSwapChainImageViews();
	m_vSwapChainFrameBuffers.resize(imageViews.size());

	for (size_t i = 0; i < imageViews.size(); i++)
	{
		std::array<VkImageView, 2> attachments =
		{
			imageViews[i],
			m_pDevice->GetDepthImageView()
		};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = m_mainForwardRenderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = m_pDevice->GetSwapChainExtent().width;
		framebufferInfo.height = m_pDevice->GetSwapChainExtent().height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(m_pDevice->GetLogicalDevice(), &framebufferInfo, nullptr, &m_vSwapChainFrameBuffers[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create framebuffer!");
		}
	}
}


void icpForwardSceneRenderer::AllocateCommandBuffers()
{
	m_vMainForwardCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

	VkCommandBufferAllocateInfo gAllocInfo{};
	gAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	gAllocInfo.commandPool = m_pDevice->GetGraphicsCommandPool();
	gAllocInfo.commandBufferCount = (uint32_t)m_vMainForwardCommandBuffers.size();
	gAllocInfo.level = VkCommandBufferLevel::VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	if (vkAllocateCommandBuffers(m_pDevice->GetLogicalDevice(), &gAllocInfo, m_vMainForwardCommandBuffers.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate graphics command buffer!");
	}
}

void icpForwardSceneRenderer::RecreateSwapChain()
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
	CreateSwapChainFrameBuffers();
}

void icpForwardSceneRenderer::CleanupSwapChain()
{
	for (auto framebuffer : m_vSwapChainFrameBuffers)
	{
		vkDestroyFramebuffer(m_pDevice->GetLogicalDevice(), framebuffer, nullptr);
	}
}

void icpForwardSceneRenderer::ResetThenBeginCommandBuffer()
{
	vkResetCommandBuffer(m_vMainForwardCommandBuffers[m_currentFrame], 0);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	if (vkBeginCommandBuffer(m_vMainForwardCommandBuffers[m_currentFrame], &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("failed to begin recording command buffer!");
	}
}

void icpForwardSceneRenderer::BeginForwardRenderPass(uint32_t imageIndex)
{
	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = m_mainForwardRenderPass;
	renderPassInfo.framebuffer = m_vSwapChainFrameBuffers[imageIndex];
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = m_pDevice->GetSwapChainExtent();

	std::array<VkClearValue, 2> clearColors{};
	clearColors[0].color = { {0.f,0.f,0.f,1.f} };
	clearColors[1].depthStencil = { 1.f, 0 };

	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearColors.size());
	renderPassInfo.pClearValues = clearColors.data();

	vkCmdBeginRenderPass(m_vMainForwardCommandBuffers[m_currentFrame], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void icpForwardSceneRenderer::EndForwardRenderPass()
{
	vkCmdEndRenderPass(m_vMainForwardCommandBuffers[m_currentFrame]);
}


void icpForwardSceneRenderer::EndRecordingCommandBuffer()
{
	if (vkEndCommandBuffer(m_vMainForwardCommandBuffers[m_currentFrame]) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to record command buffer!");
	}
}


INCEPTION_END_NAMESPACE