#include "icpUiPass.h"

#include "../../core/icpSystemContainer.h"
#include "../../render/icpWindowSystem.h"
#include "../../render/icpRenderSystem.h"
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

	m_dependency = initInfo.dependency;
	m_renderPassObj = initInfo.dependency->m_renderPassObj;

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
}

void icpUiPass::setupPipeline()
{
	
}

void icpUiPass::render()
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();

	ImGui::NewFrame();

	ImGui::Render();

	auto mainPass = dynamic_pointer_cast<icpMainForwardPass>(m_dependency);

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), m_rhi->m_graphicsCommandBuffers[mainPass->m_currentFrame]);
}

void icpUiPass::cleanup()
{
	
}


INCEPTION_END_NAMESPACE