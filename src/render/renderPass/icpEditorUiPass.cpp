#include "icpEditorUiPass.h"
#include "../../render/RHI/Vulkan/icpVulkanUtility.h"
#include "../../mesh/icpMeshData.h"
#include "../../core/icpSystemContainer.h"
#include "../../core/icpConfigSystem.h"
#include "../../mesh/icpMeshResource.h"
#include "../../resource/icpResourceSystem.h"

#include <backends/imgui_impl_vulkan.h>
#include <backends/imgui_impl_glfw.h>

#include "../../ui/editorUI/icpEditorUI.h"

INCEPTION_BEGIN_NAMESPACE

icpEditorUiPass::~icpEditorUiPass()
{

}

void icpEditorUiPass::initializeRenderPass(RenderPassInitInfo initInfo)
{
	m_rhi = initInfo.device;

	m_editorUI = initInfo.editorUi;

	createRenderPass();
	setupPipeline();
	createFrameBuffers();
	//																				POI : may cause problem
	VkCommandBuffer command_buffer = icpVulkanUtility::beginSingleTimeCommands(m_rhi->GetGraphicsCommandPool(), m_rhi->GetLogicalDevice());
	ImGui_ImplVulkan_CreateFontsTexture(command_buffer);
	icpVulkanUtility::endSingleTimeCommandsAndSubmit(command_buffer, m_rhi->GetGraphicsQueue(), m_rhi->GetGraphicsCommandPool(), m_rhi->GetLogicalDevice());
}

void icpEditorUiPass::createFrameBuffers()
{
	auto& swapChainViews = m_rhi->GetSwapChainImageViews();
	m_swapChainFramebuffers.resize(swapChainViews.size());

	for (uint32_t i = 0; i < swapChainViews.size(); i++)
	{
		VkImageView attachment[1];
		attachment[0] = swapChainViews[i];
		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = m_renderPassObj;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = attachment;
		framebufferInfo.width = m_rhi->GetSwapChainExtent().width;
		framebufferInfo.height = m_rhi->GetSwapChainExtent().height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(m_rhi->GetLogicalDevice(), &framebufferInfo, nullptr, &m_swapChainFramebuffers[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create framebuffer!");
		}
	}
}

void icpEditorUiPass::cleanup()
{
	
}

void icpEditorUiPass::recreateSwapChain()
{
	for (auto framebuffer : m_swapChainFramebuffers)
	{
		vkDestroyFramebuffer(m_rhi->GetLogicalDevice(), framebuffer, nullptr);
	}
	createFrameBuffers();
}


void icpEditorUiPass::createRenderPass()
{
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = m_rhi->GetSwapChainImageFormat();
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	std::array<VkAttachmentDescription, 1> attachments{ colorAttachment };

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(m_rhi->GetLogicalDevice(), &renderPassInfo, nullptr, &m_renderPassObj) != VK_SUCCESS) {
		throw std::runtime_error("failed to create render pass!");
	}
}

void icpEditorUiPass::render(uint32_t frameBufferIndex, uint32_t currentFrame, VkResult acquireImageResult, VkSubmitInfo& info)
{
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	//ImGui::ShowDemoWindow();
	m_editorUI->showEditorUI();

	ImGui::Render();

	auto& graphicsCBs = m_rhi->GetGraphicsCommandBuffers();

	vkResetCommandBuffer(graphicsCBs[currentFrame], 0);
	recordCommandBuffer(graphicsCBs[currentFrame], frameBufferIndex);

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), graphicsCBs[currentFrame]);

	vkCmdEndRenderPass(graphicsCBs[currentFrame]);

	VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &graphicsCBs[currentFrame];
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &m_rhi->GetRenderFinishedForPresentationSemaphores()[currentFrame];

	vkEndCommandBuffer(graphicsCBs[currentFrame]);

	info = submitInfo;
}

void icpEditorUiPass::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t curFrameIndex)
{
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("failed to begin recording command buffer!");
	}

	VkRenderPassBeginInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	info.renderPass = m_renderPassObj;
	info.framebuffer = m_swapChainFramebuffers[curFrameIndex];
	info.renderArea.extent = m_rhi->GetSwapChainExtent();
	std::array<VkClearValue, 2> clearColors{};
	clearColors[0].color = { {0.f,0.f,0.f,1.f} };
	clearColors[1].depthStencil = { 1.f, 0 };

	info.clearValueCount = static_cast<uint32_t>(clearColors.size());
	info.pClearValues = clearColors.data();
	vkCmdBeginRenderPass(commandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);

}

void icpEditorUiPass::setupPipeline()
{
	ImGui::CreateContext();

	auto io = ImGui::GetIO();

	auto window = g_system_container.m_windowSystem->getWindow();

	ImGui_ImplGlfw_InitForVulkan(window, true);

	ImGui_ImplVulkan_InitInfo info{};
	info.Device = m_rhi->GetLogicalDevice();
	info.DescriptorPool = m_rhi->GetDescriptorPool();
	info.ImageCount = 3;
	info.Instance = m_rhi->GetInstance();
	info.MinImageCount = 3;
	info.PhysicalDevice = m_rhi->GetPhysicalDevice();
	info.Queue = m_rhi->GetGraphicsQueue();
	info.QueueFamily = m_rhi->GetQueueFamilyIndices().m_graphicsFamily.value();
	info.Subpass = 0;

	ImGui_ImplVulkan_Init(&info, m_renderPassObj);
}


INCEPTION_END_NAMESPACE