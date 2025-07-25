#include "icpSceneRenderer.h"

#include "light/icpLightComponent.h"
#include "renderPass/icpRenderPassBase.h"
#include "RHI/Vulkan/icpVulkanUtility.h"

#include "icpCameraSystem.h"
#include "light/icpLightSystem.h"
#include "shadow/icpShadowManager.h"
#include "../render/icpRenderSystem.h"

INCEPTION_BEGIN_NAMESPACE

std::shared_ptr<icpRenderPassBase> icpSceneRenderer::AccessRenderPass(eRenderPass passType)
{
	if (m_renderPasses.contains(passType))
	{
		return m_renderPasses[passType];
	}

	return nullptr;
}

void icpSceneRenderer::Cleanup()
{
	for (const auto renderPass : m_renderPasses)
	{
		renderPass.second->Cleanup();
	}
}

void icpSceneRenderer::CreateSceneCB()
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

void icpSceneRenderer::UpdateGlobalSceneCB(uint32_t curFrame)
{
	perFrameCB CBPerFrame{};

	auto aspectRatio = (float)m_pDevice->GetSwapChainExtent().width / (float)m_pDevice->GetSwapChainExtent().height;
	auto cameraSys = g_system_container.m_cameraSystem;
	cameraSys->UpdateCameraCB(CBPerFrame, aspectRatio);

	auto lightSys = g_system_container.m_lightSystem;
	lightSys->UpdateLightCB(CBPerFrame);

	void* data;
	vmaMapMemory(m_pDevice->GetVmaAllocator(), m_vSceneCBAllocations[curFrame], &data);
	memcpy(data, &CBPerFrame, sizeof(perFrameCB));
	vmaUnmapMemory(m_pDevice->GetVmaAllocator(), m_vSceneCBAllocations[curFrame]);
}

void icpSceneRenderer::UpdateCSMProjViewMat(uint32_t curFrame)
{
	auto aspectRatio = (float)m_pDevice->GetSwapChainExtent().width / (float)m_pDevice->GetSwapChainExtent().height;

	auto directionalLightView = g_system_container.m_sceneSystem->m_registry.view<icpDirectionalLightComponent>();
	glm::vec3 directionalLightDir;
	for (auto& light : directionalLightView)
	{
		auto& lightComp = directionalLightView.get<icpDirectionalLightComponent>(light);
		directionalLightDir = lightComp.m_direction;
	}

	auto shadowSys = g_system_container.m_renderSystem->m_shadowManager;
	shadowSys->UpdateCSMProjViewMat(aspectRatio, directionalLightDir, curFrame);
}


void icpSceneRenderer::CreateGlobalSceneDescriptorSetLayout()
{
	m_sceneDSLayout = DescriptorSetLayoutBuilder()
		.SetDescriptorSetBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
		.Build(m_pDevice->GetLogicalDevice());

}

VkDescriptorSetLayout icpSceneRenderer::GetSceneDSLayout()
{
	return m_sceneDSLayout;
}


uint32_t icpSceneRenderer::GetCurrentFrame() const
{
	return m_currentFrame;
}


INCEPTION_END_NAMESPACE