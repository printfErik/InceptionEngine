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
	mainForwordPass->InitializeRenderPass(mainPassCreateInfo);

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
	editorUIPass->InitializeRenderPass(editorUIInfo);

	m_renderPasses.push_back(editorUIPass);

	return true;
}

void icpRenderPassManager::cleanup()
{
	for (const auto renderPass: m_renderPasses)
	{
		renderPass->Cleanup();
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
		renderPass->Render(index, m_currentFrame, result, submitInfo);
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
			renderPass->RecreateSwapChain();
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

void icpRenderPassManager::CreateGlobalSceneDescriptorSetLayout()
{
	// perFrame
	{
		VkDescriptorSetLayoutBinding perFrameUBOBinding{};
		perFrameUBOBinding.binding = 0;
		perFrameUBOBinding.descriptorCount = 1;
		perFrameUBOBinding.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		perFrameUBOBinding.pImmutableSamplers = nullptr;
		perFrameUBOBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		m_sceneDSLayout.bindings.push_back({ VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER });

		std::array<VkDescriptorSetLayoutBinding, 1> bindings{ perFrameUBOBinding };

		VkDescriptorSetLayoutCreateInfo createInfo{};
		createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		createInfo.pBindings = bindings.data();
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

		if (vkCreateDescriptorSetLayout(m_pDevice->GetLogicalDevice(), &createInfo, nullptr, &m_sceneDSLayout.layout) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create descriptor set layout!");
		}
	}
}


void icpRenderPassManager::AllocateGlobalSceneDescriptorSets()
{
	icpDescriptorSetCreation creation{};
	creation.layoutInfo = m_sceneDSLayout;

	std::vector<icpBufferRenderResourceInfo> bufferInfos;
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		icpBufferRenderResourceInfo bufferInfo{};
		bufferInfo.buffer = m_vSceneCBs[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(perFrameCB);
		bufferInfos.push_back(bufferInfo);
	}

	creation.SetUniformBuffer(0, bufferInfos);
	m_pDevice->CreateDescriptorSet(creation, m_sceneDSs);
}

void icpRenderPassManager::CreateForwardRenderPass()
{
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = m_pDevice->GetSwapChainImageFormat();
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

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

	if (vkCreateRenderPass(m_pDevice->GetLogicalDevice(), &renderPassInfo, nullptr, &m_renderPassObj) != VK_SUCCESS) {
		throw std::runtime_error("failed to create render pass!");
	}
}
INCEPTION_END_NAMESPACE