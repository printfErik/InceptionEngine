#include "icpSceneRenderer.h"

#include "light/icpLightComponent.h"
#include "renderPass/icpRenderPassBase.h"
#include "RHI/Vulkan/icpVulkanUtility.h"

#include "icpCameraSystem.h"

INCEPTION_BEGIN_NAMESPACE

std::shared_ptr<icpRenderPassBase> icpSceneRenderer::AccessRenderPass(eRenderPass passType)
{
	return m_renderPasses[static_cast<int>(passType)];
}

void icpSceneRenderer::Cleanup()
{
	for (const auto renderPass : m_renderPasses)
	{
		renderPass->Cleanup();
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

void icpSceneRenderer::CreateGlobalSceneDescriptorSetLayout()
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

void icpSceneRenderer::AllocateGlobalSceneDescriptorSets()
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
	m_pDevice->CreateDescriptorSet(creation, m_vSceneDSs);
}


INCEPTION_END_NAMESPACE