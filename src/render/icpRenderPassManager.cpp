#include "icpRenderPassManager.h"
#include "icpVulkanRHI.h"
#include "icpVulkanUtility.h"
#include "renderPass/icpMainForwardPass.h"
#include "renderPass/icpUiPass.h"

#include <vulkan/vulkan.hpp>

#include "renderPass/icpRenderPassBase.h"

INCEPTION_BEGIN_NAMESPACE

icpRenderPassManager::icpRenderPassManager()
{
}


icpRenderPassManager::~icpRenderPassManager()
{
	cleanup();
}

bool icpRenderPassManager::initialize(std::shared_ptr<icpVulkanRHI> vulkanRHI)
{
	m_rhi = vulkanRHI;

	icpRenderPassBase::RendePassInitInfo mainPassCreateInfo;
	mainPassCreateInfo.rhi = m_rhi;
	mainPassCreateInfo.passType = eRenderPass::MAIN_FORWARD_PASS;
	std::shared_ptr<icpRenderPassBase> mainForwordPass = std::make_shared<icpMainForwardPass>();
	mainForwordPass->initializeRenderPass(mainPassCreateInfo);

	m_renderPasses.push_back(mainForwordPass);

	icpRenderPassBase::RendePassInitInfo uiPassInfo;
	uiPassInfo.rhi = m_rhi;
	uiPassInfo.passType = eRenderPass::UI_PASS;
	uiPassInfo.dependency = m_renderPasses[0];
	std::shared_ptr<icpRenderPassBase> uiPass = std::make_shared<icpUiPass>();
	uiPass->initializeRenderPass(uiPassInfo);

	m_renderPasses.push_back(uiPass);

	return true;
}

void icpRenderPassManager::cleanup()
{
	for (const auto renderPass: m_renderPasses)
	{
		renderPass->cleanup();
	}
}

void icpRenderPassManager::render()
{
	VkResult result;
	auto index = m_rhi->acquireNextImageFromSwapchain(m_currentFrame, result);

	m_rhi->waitForFence(m_currentFrame);

	std::vector<VkSubmitInfo> infos;

	for(const auto renderPass : m_renderPasses)
	{
		VkSubmitInfo submitInfo;
		renderPass->render(index, m_currentFrame, result, submitInfo);
		infos.push_back(submitInfo);
	}

	result = vkQueueSubmit(m_rhi->m_graphicsQueue, static_cast<uint32_t>(infos.size()), infos.data(), m_rhi->m_inFlightFences[m_currentFrame]);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_rhi->m_framebufferResized) {
		//recreateSwapChain();
		m_rhi->m_framebufferResized = false;
	}
	else if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to present swap chain image!");
	}

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &m_rhi->m_renderFinishedForPresentationSemaphores[m_currentFrame];

	VkSwapchainKHR swapChains[] = { m_rhi->m_swapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;

	presentInfo.pImageIndices = &index;

	vkQueuePresentKHR(m_rhi->m_presentQueue, &presentInfo);

	m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}



INCEPTION_END_NAMESPACE