#include "icpRenderPassManager.h"
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

	CreateSceneCB();

	icpRenderPassBase::RenderPassInitInfo mainPassCreateInfo;
	mainPassCreateInfo.device = m_pDevice;
	mainPassCreateInfo.passType = eRenderPass::MAIN_FORWARD_PASS;
	mainPassCreateInfo.renderPassMgr = shared_from_this();
	std::shared_ptr<icpRenderPassBase> mainForwordPass = std::make_shared<icpMainForwardPass>();
	mainForwordPass->initializeRenderPass(mainPassCreateInfo);

	m_renderPasses.push_back(mainForwordPass);

	/*
	icpRenderPassBase::RenderPassInitInfo unlitPassInfo;
	copyPassInfo.rhi = m_rhi;
	copyPassInfo.passType = eRenderPass::COPY_PASS;
	std::shared_ptr<icpRenderPassBase> copyPass = std::make_shared<icpRenderToImgPass>();
	copyPass->initializeRenderPass(copyPassInfo);

	m_renderPasses.push_back(copyPass);
	*/

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

	UpdateGlobalSceneCB(m_currentFrame);

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

void icpRenderPassManager::CreateSceneCB()
{
	auto perFrameSize = sizeof(perFrameCB);
	VkSharingMode mode = m_pDevice->GetQueueFamilyIndices().m_graphicsFamily.value() == m_pDevice->GetQueueFamilyIndices().m_transferFamily.value() ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;

	m_vSceneCBs.resize(MAX_FRAMES_IN_FLIGHT);
	m_vSceneCBAllocations.resize(MAX_FRAMES_IN_FLIGHT);

	auto allocator = m_pDevice->GetVmaAllocator();
	auto& queueIndices = m_pDevice->GetQueueFamilyIndicesVector();

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		icpVulkanUtility::CreateGPUBuffer(
			perFrameSize,
			mode,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			allocator,
			m_vSceneCBAllocations[i],
			m_vSceneCBs[i],
			queueIndices.size(),
			queueIndices.data()
		);
	}
}

void icpRenderPassManager::UpdateGlobalSceneCB(uint32_t curFrame)
{
	auto camera = g_system_container.m_cameraSystem->getCurrentCamera();

	perFrameCB CBPerFrame{};

	CBPerFrame.view = g_system_container.m_cameraSystem->getCameraViewMatrix(camera);
	auto aspectRatio = (float)m_pDevice->GetSwapChainExtent().width / (float)m_pDevice->GetSwapChainExtent().height;
	CBPerFrame.projection = glm::perspective(camera->m_fov, aspectRatio, camera->m_near, camera->m_far);
	CBPerFrame.projection[1][1] *= -1;

	CBPerFrame.camPos = camera->m_position;

	auto lightView = g_system_container.m_sceneSystem->m_registry.view<icpDirectionalLightComponent>();

	int index = 0;
	for (auto& light : lightView)
	{
		auto& lightComp = lightView.get<icpDirectionalLightComponent>(light);
		auto dirL = dynamic_cast<icpDirectionalLightComponent&>(lightComp);
		CBPerFrame.dirLight.color = glm::vec4(dirL.m_color, 1.f);
		CBPerFrame.dirLight.direction = glm::vec4(dirL.m_direction, 0.f);

		// todo add point lights
	}

	CBPerFrame.pointLightNumber = 0.f;

	if (!lightView.empty())
	{
		void* data;
		vmaMapMemory(m_pDevice->GetVmaAllocator(), m_vSceneCBAllocations[curFrame], &data);
		memcpy(data, &CBPerFrame, sizeof(perFrameCB));
		vmaUnmapMemory(m_pDevice->GetVmaAllocator(), m_vSceneCBAllocations[curFrame]);
	}
}


INCEPTION_END_NAMESPACE