#include "icpUiPass.h"

#include "../../core/icpSystemContainer.h"
#include "../../render/icpWindowSystem.h"
#include "../../render/icpRenderSystem.h"
#include "../../render/icpVulkanUtility.h"
#include "icpMainForwardPass.h"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

INCEPTION_BEGIN_NAMESPACE

icpUiPass::~icpUiPass()
{
	
}


void icpUiPass::initializeRenderPass(RendePassInitInfo initInfo)
{

	m_rhi = initInfo.rhi;

	createRenderPass();

	ImGui::CreateContext();

	auto io = ImGui::GetIO();

	auto window = g_system_container.m_windowSystem->getWindow();

	ImGui_ImplGlfw_InitForVulkan(window, true);

	ImGui_ImplVulkan_InitInfo info{};
	info.Device = m_rhi->m_device;
	info.DescriptorPool = m_rhi->m_descriptorPool;
	info.ImageCount = 3;
	info.Instance = m_rhi->m_instance;
	info.MinImageCount = 3;
	info.PhysicalDevice = m_rhi->m_physicalDevice;
	info.Queue = m_rhi->m_graphicsQueue;
	info.QueueFamily = m_rhi->m_queueIndices.m_graphicsFamily.value();
	info.Subpass = static_cast<uint32_t>(eRenderPass::UI_PASS);

	ImGui_ImplVulkan_Init(&info, m_renderPassObj);

	createFrameBuffers();

	VkCommandBuffer command_buffer = icpVulkanUtility::beginSingleTimeCommands(m_rhi->m_uiCommandPool, m_rhi->m_device);
	ImGui_ImplVulkan_CreateFontsTexture(command_buffer);
	icpVulkanUtility::endSingleTimeCommandsAndSubmit(command_buffer, m_rhi->m_graphicsQueue, m_rhi->m_uiCommandPool, m_rhi->m_device);


}

void icpUiPass::setupPipeline()
{
	
}

void icpUiPass::createFrameBuffers()
{
	VkImageView attachment[1];
	VkFramebufferCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	info.renderPass = m_renderPassObj;
	info.attachmentCount = 1;
	info.pAttachments = attachment;
	info.width = m_rhi->m_swapChainExtent.width;
	info.height = m_rhi->m_swapChainExtent.height;
	info.layers = 1;
	for (uint32_t i = 0; i < m_rhi->m_swapChainImageViews.size(); i++)
	{
		attachment[0] = m_rhi->m_swapChainImageViews[i];
		if(vkCreateFramebuffer(m_rhi->m_device, &info, nullptr, &m_swapChainFramebuffers[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create framebuffer!");
		}
	}
}


void icpUiPass::render()
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	ImGui::ShowDemoWindow();
	ImGui::Render();

	m_rhi->waitForFence(m_currentFrame);
	VkResult result;
	auto index = m_rhi->acquireNextImageFromSwapchain(m_currentFrame, result);

	recordCommandBuffer(m_currentFrame);

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), m_rhi->m_uiCommandBuffers[index]);
}

void icpUiPass::recordCommandBuffer(uint32_t curFrameIndex)
{
	VkRenderPassBeginInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	info.renderPass = m_renderPassObj;
	info.framebuffer = m_swapChainFramebuffers[curFrameIndex];
	info.renderArea.extent = m_rhi->m_swapChainExtent;
	std::array<VkClearValue, 2> clearColors{};
	clearColors[0].color = { {0.f,0.f,0.f,1.f} };
	clearColors[1].depthStencil = { 1.f, 0 };

	info.clearValueCount = static_cast<uint32_t>(clearColors.size());
	info.pClearValues = clearColors.data();
	vkCmdBeginRenderPass(m_rhi->m_uiCommandBuffers[curFrameIndex], &info, VK_SUBPASS_CONTENTS_INLINE);
}


void icpUiPass::cleanup()
{
	
}

void icpUiPass::createRenderPass()
{
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = m_rhi->m_swapChainImageFormat;
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

	if (vkCreateRenderPass(m_rhi->m_device, &renderPassInfo, nullptr, &m_renderPassObj) != VK_SUCCESS) {
		throw std::runtime_error("failed to create render pass!");
	}
}


INCEPTION_END_NAMESPACE