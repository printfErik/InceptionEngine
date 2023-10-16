#include "icpRenderPassManager.h"
#include "RHI/Vulkan/icpVkGPUDevice.h"
#include "RHI/Vulkan/icpVulkanUtility.h"
#include "renderPass/icpMainForwardPass.h"
#include "renderPass/icpEditorUiPass.h"

#include <vulkan/vulkan.hpp>

#include "../ui/editorUI/icpEditorUI.h"
#include "renderPass/icpRenderPassBase.h"

INCEPTION_BEGIN_NAMESPACE

icpRenderPassManager::icpRenderPassManager()
{
}


icpRenderPassManager::~icpRenderPassManager()
{
	cleanup();
}

bool icpRenderPassManager::initialize(std::shared_ptr<icpGPUDevice> vulkanRHI)
{
	m_pDevice = vulkanRHI;

	icpRenderPassBase::RenderPassInitInfo mainPassCreateInfo;
	mainPassCreateInfo.device = m_pDevice;
	mainPassCreateInfo.passType = eRenderPass::MAIN_FORWARD_PASS;
	std::shared_ptr<icpRenderPassBase> mainForwordPass = std::make_shared<icpMainForwardPass>();
	mainForwordPass->initializeRenderPass(mainPassCreateInfo);

	m_renderPasses.push_back(mainForwordPass);

	
	icpRenderPassBase::RenderPassInitInfo unlitPassInfo;
	copyPassInfo.rhi = m_rhi;
	copyPassInfo.passType = eRenderPass::COPY_PASS;
	std::shared_ptr<icpRenderPassBase> copyPass = std::make_shared<icpRenderToImgPass>();
	copyPass->initializeRenderPass(copyPassInfo);

	m_renderPasses.push_back(copyPass);
	

	icpRenderPassBase::RenderPassInitInfo editorUIInfo;
	editorUIInfo.device = m_pDevice;
	editorUIInfo.passType = eRenderPass::EDITOR_UI_PASS;
	editorUIInfo.editorUi = std::make_shared<icpEditorUI>();
	std::shared_ptr<icpRenderPassBase> editorUIPass = std::make_shared<icpEditorUiPass>();
	editorUIPass->initializeRenderPass(editorUIInfo);

	m_renderPasses.push_back(editorUIPass);

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
	m_pDevice->WaitForFence(m_currentFrame);

	VkResult result;
	auto index = m_pDevice->AcquireNextImageFromSwapchain(m_currentFrame, result);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		return;
	}

	std::vector<VkSubmitInfo> infos;

	for(const auto renderPass : m_renderPasses)
	{
		VkSubmitInfo submitInfo;
		renderPass->render(index, m_currentFrame, result, submitInfo);
		infos.push_back(submitInfo);
	}

	auto& fences = m_pDevice->GetInFlightFences();

	result = vkQueueSubmit(m_pDevice->GetGraphicsQueue(), static_cast<uint32_t>(infos.size()), infos.data(), fences[m_currentFrame]);

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;

	auto& RenderFinishedForPresentationSemaphores = m_pDevice->GetRenderFinishedForPresentationSemaphores();
	presentInfo.pWaitSemaphores = &RenderFinishedForPresentationSemaphores[m_currentFrame];

	VkSwapchainKHR swapChains[] = { m_pDevice->GetSwapChain() };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;

	presentInfo.pImageIndices = &index;

	vkQueuePresentKHR(m_pDevice->GetPresentQueue(), &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_pDevice->m_framebufferResized)
	{
		for (const auto renderPass : m_renderPasses)
		{
			renderPass->recreateSwapChain();
		}
		m_pDevice->m_framebufferResized = false;
	}
	else if (result != VK_SUCCESS) 
	{
		throw std::runtime_error("failed to present swap chain image!");
	}

	m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

std::shared_ptr<icpRenderPassBase> icpRenderPassManager::accessRenderPass(eRenderPass passType)
{
	return m_renderPasses[static_cast<int>(passType)];
}

INCEPTION_END_NAMESPACE