#include "icpRenderPipeline.h"
#include "icpVulkanRHI.h"
#include "icpVulkanUtility.h"
#include "../core/icpSystemContainer.h"
#include "../core/icpConfigSystem.h"
#include "../mesh/icpMeshResource.h"
#include "../resource/icpResourceSystem.h"

#include <vulkan/vulkan.hpp>
#include <fstream>
#include <iterator>

INCEPTION_BEGIN_NAMESPACE

icpRenderPipeline::icpRenderPipeline()
{
}


icpRenderPipeline::~icpRenderPipeline()
{
	cleanup();
}

void icpRenderPipeline::cleanup()
{
	for (auto& shader : m_shaderModules)
	{
		vkDestroyShaderModule(m_rhi->m_device, shader, nullptr);
	}

	cleanupSwapChain();

	vkDestroyRenderPass(m_rhi->m_device, m_renderPass, nullptr);
	vkDestroyPipelineLayout(m_rhi->m_device, m_pipelineLayout, nullptr);
	vkDestroyPipeline(m_rhi->m_device, m_pipeline, nullptr);
}

void icpRenderPipeline::cleanupSwapChain()
{
	for (auto framebuffer : m_swapChainFramebuffers) 
	{
		vkDestroyFramebuffer(m_rhi->m_device, framebuffer, nullptr);
	}
}

bool icpRenderPipeline::initialize(std::shared_ptr<icpVulkanRHI> vulkanRHI)
{
	m_rhi = vulkanRHI;

	createRenderPass();
	createGraphicsPipeline();
	createFrameBuffers();

	return true;
}

void icpRenderPipeline::createGraphicsPipeline()
{
	
}

VkShaderModule icpRenderPipeline::createShaderModule(const char* shaderFileName)
{
	std::ifstream inFile(shaderFileName, std::ios::binary | std::ios::ate);
	size_t fileSize = (size_t)inFile.tellg();
	std::vector<char> content(fileSize);

	inFile.seekg(0);
	inFile.read(content.data(), fileSize);

	inFile.close();

	VkShaderModuleCreateInfo creatInfo{};
	creatInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	creatInfo.pCode = reinterpret_cast<const uint32_t*>(content.data());
	creatInfo.codeSize = content.size();

	VkShaderModule shader{ VK_NULL_HANDLE };
	if (vkCreateShaderModule(m_rhi->m_device, &creatInfo, nullptr, &shader) != VK_SUCCESS)
	{
		throw std::runtime_error("create shader module failed");
	}

	m_shaderModules.push_back(shader);
	return m_shaderModules.back();
}

void icpRenderPipeline::createRenderPass()
{

}

void icpRenderPipeline::createFrameBuffers()
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
		framebufferInfo.renderPass = m_renderPass;
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

void icpRenderPipeline::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("failed to begin recording command buffer!");
	}

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = m_renderPass;
	renderPassInfo.framebuffer = m_swapChainFramebuffers[imageIndex];
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = m_rhi->m_swapChainExtent;

	std::array<VkClearValue, 2> clearColors{};
	clearColors[0].color = { {0.f,0.f,0.f,1.f} };
	clearColors[1].depthStencil = { 1.f, 0 };

	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearColors.size());
	renderPassInfo.pClearValues = clearColors.data();

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

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

	std::vector<VkBuffer> vertexBuffers{ m_rhi->m_vertexBuffer };
	std::vector<VkDeviceSize> offsets{ 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers.data(), offsets.data());

	vkCmdBindIndexBuffer(commandBuffer, m_rhi->m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);

	vkCmdBindDescriptorSets(commandBuffer, VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_rhi->m_descriptorSets[m_currentFrame], 0, nullptr);
	auto meshP = std::dynamic_pointer_cast<icpMeshResource>(g_system_container.m_resourceSystem->m_resources.m_allResources["viking_room"]);
	vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(meshP->m_meshData.m_vertexIndices.size()), 1, 0, 0, 0);

	vkCmdEndRenderPass(commandBuffer);

	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) 
	{
		throw std::runtime_error("failed to record command buffer!");
	}
}

void icpRenderPipeline::render()
{
	m_rhi->waitForFence(m_currentFrame);
	VkResult result;
	auto index = m_rhi->acquireNextImageFromSwapchain(m_currentFrame, result);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		recreateSwapChain();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		throw std::runtime_error("failed to present swap chain image!");
	}

	m_rhi->updateUniformBuffers(m_currentFrame);

	vkResetFences(m_rhi->m_device, 1, &m_rhi->m_inFlightFences[m_currentFrame]);

	m_rhi->resetCommandBuffer(m_currentFrame);
	recordCommandBuffer(m_rhi->m_graphicsCommandBuffers[m_currentFrame], index);
	result = m_rhi->submitRendering(index, m_currentFrame);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_rhi->m_framebufferResized) {
		recreateSwapChain();
		m_rhi->m_framebufferResized = false;
	}
	else if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to present swap chain image!");
	}

	m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void icpRenderPipeline::recreateSwapChain() {

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


INCEPTION_END_NAMESPACE