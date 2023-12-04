#include "icpEditorUiPass.h"
#include "../../render/RHI/Vulkan/icpVulkanUtility.h"
#include "../../mesh/icpMeshData.h"
#include "../../core/icpSystemContainer.h"
#include "../../core/icpConfigSystem.h"
#include "../../mesh/icpMeshResource.h"
#include "../../resource/icpResourceSystem.h"

#include <backends/imgui_impl_vulkan.h>
#include <backends/imgui_impl_glfw.h>

#include "../icpSceneRenderer.h"
#include "../../ui/editorUI/icpEditorUI.h"

INCEPTION_BEGIN_NAMESPACE

icpEditorUiPass::~icpEditorUiPass()
{

}

void icpEditorUiPass::InitializeRenderPass(RenderPassInitInfo initInfo)
{
	m_rhi = initInfo.device;
	m_editorUI = initInfo.editorUi;
	m_pSceneRenderer = initInfo.sceneRenderer;

	SetupPipeline();

	VkCommandBuffer command_buffer = icpVulkanUtility::beginSingleTimeCommands(m_rhi->GetGraphicsCommandPool(), m_rhi->GetLogicalDevice());
	ImGui_ImplVulkan_CreateFontsTexture(command_buffer);
	icpVulkanUtility::endSingleTimeCommandsAndSubmit(command_buffer, m_rhi->GetGraphicsQueue(), m_rhi->GetGraphicsCommandPool(), m_rhi->GetLogicalDevice());
}

void icpEditorUiPass::Cleanup()
{
	
}

void icpEditorUiPass::Render(uint32_t frameBufferIndex, uint32_t currentFrame, VkResult acquireImageResult)
{
	auto mgr = m_pSceneRenderer.lock();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	//ImGui::ShowDemoWindow();
	m_editorUI->showEditorUI();

	ImGui::Render();

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), mgr->GetMainForwardCommandBuffer(currentFrame));
}

void icpEditorUiPass::SetupPipeline()
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

	ImGui_ImplVulkan_Init(&info, m_pSceneRenderer.lock()->GetMainForwardRenderPass());
}


INCEPTION_END_NAMESPACE