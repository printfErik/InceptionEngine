#include "../core/icpSystemContainer.h"

#include "icpUiSystem.h"

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
	ImGui_ImplVulkan_Init()
}


INCEPTION_END_NAMESPACE