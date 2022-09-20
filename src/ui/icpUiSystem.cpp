#include "../core/icpSystemContainer.h"

#include "icpUiSystem.h"
#include "../render/icpRenderSystem.h"

#include <imgui.h>

#include "../render/icpWindowSystem.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"

INCEPTION_BEGIN_NAMESPACE

void icpUiSystem::initializeUiCanvas()
{
	
	ImGui::CreateContext();
	
	auto io = ImGui::GetIO();

	auto window = g_system_container.m_windowSystem->getWindow();

	ImGui_ImplGlfw_InitForVulkan(window, true);

	const auto& renderSystem = g_system_container.m_renderSystem;
	const auto& vulkanRHI = dynamic_pointer_cast<icpVulkanRHI>(renderSystem->m_rhi);
	const auto& pipeline = renderSystem->m_renderPipeline;

	ImGui_ImplVulkan_InitInfo info{};
	info.Device = vulkanRHI->m_device;
	info.DescriptorPool = vulkanRHI->m_descriptorPool;
	info.ImageCount = 3;
	info.Instance = vulkanRHI->m_instance;
	info.MinImageCount = 3;
	info.PhysicalDevice = vulkanRHI->m_physicalDevice;
	info.Queue = vulkanRHI->m_graphicsQueue;
	info.QueueFamily = vulkanRHI->m_queueIndices.m_graphicsFamily.value();
	info.Subpass = 0;

	ImGui_ImplVulkan_Init(&info, pipeline->m_renderPass);
	
}


INCEPTION_END_NAMESPACE