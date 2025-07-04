#include "icpShadowManager.h"
#include <glm/gtc/matrix_transform.hpp>
#include "../icpCameraSystem.h"
#include "../../core/icpConfigSystem.h"
#include "../../core/icpSystemContainer.h"
#include "../RHI/Vulkan/icpVulkanUtility.h"

INCEPTION_BEGIN_NAMESPACE

void icpShadowManager::InitCascadeDistance()
{
    // Split View Frustum
    m_cascadeSplits.resize(s_csmCascadeCount + 1);

    float nearPlane = g_system_container.m_configSystem->NearPlane;
    float farPlane = g_system_container.m_configSystem->FarPlane;

    m_cascadeSplits[0] = nearPlane;
    m_cascadeSplits[s_csmCascadeCount] = farPlane;
    for (int i = 1; i < s_csmCascadeCount; ++i) 
    {
        float si = (float)i / s_csmCascadeCount;
        float log = nearPlane * std::pow(farPlane / nearPlane, si);
        float lin = nearPlane + (farPlane - nearPlane) * si;
        float d = 0.5f * (log - lin) + lin;
        m_cascadeSplits[i] = d;
    }

    m_lightProjViews.resize(s_csmCascadeCount);
}


glm::vec3 computeAABBMin(const std::vector<glm::vec3>& pts)
{
    glm::vec3 mn = pts[0];
    for (size_t i = 1; i < pts.size(); ++i) 
    {
        mn = glm::min(mn, pts[i]);
    }
    return mn;
}
glm::vec3 computeAABBMax(const std::vector<glm::vec3>& pts)
{
    glm::vec3 mx = pts[0];
    for (size_t i = 1; i < pts.size(); ++i) 
    {
        mx = glm::max(mx, pts[i]);
    }
    return mx;
}

void icpShadowManager::UpdateCSMProjViewMat(float aspectRatio, const glm::vec3& direction, uint32_t curFrame)
{
    auto camera = g_system_container.m_cameraSystem->getCurrentCamera();
    auto viewMat = camera->m_viewMatrix;

    auto invViewMat = glm::inverse(viewMat);

	// Camera space 8 points
    std::vector<std::vector<glm::vec3>> cascadePointsWS;
	for (uint32_t i = 0; i < s_csmCascadeCount; i++)
	{
        auto near = 0.f - m_cascadeSplits[i];
        auto far = 0.f - m_cascadeSplits[i + 1];
        auto halfNearHeight = glm::tan(camera->m_fov / 2.f) * (- near);
        auto halfNearWidth = halfNearHeight * aspectRatio;

        auto halfFarHeight = glm::tan(camera->m_fov / 2.f) * (- far);
        auto halfFarWidth = halfFarHeight * aspectRatio;

        // To world space
        std::vector<glm::vec3> pointsWS{
            invViewMat * glm::vec4(halfNearWidth, halfNearHeight, near, 1.f),
            invViewMat * glm::vec4(-halfNearWidth, halfNearHeight, near, 1.f),
            invViewMat * glm::vec4(-halfNearWidth, -halfNearHeight, near, 1.f),
            invViewMat * glm::vec4(halfNearWidth,  -halfNearHeight, near, 1.f),
            invViewMat * glm::vec4(halfFarWidth, halfFarHeight, far, 1.f),
        	invViewMat * glm::vec4(-halfFarWidth, halfFarHeight, far, 1.f),
        	invViewMat * glm::vec4(-halfFarWidth, -halfFarHeight, far, 1.f),
        	invViewMat * glm::vec4(halfFarWidth, -halfFarHeight, far, 1.f)
        };

        cascadePointsWS.push_back(pointsWS);
	}


    for (uint32_t i = 0; i < s_csmCascadeCount; i++)
    {
        glm::vec3 aabbMin = computeAABBMin(cascadePointsWS[i]);
        glm::vec3 aabbMax = computeAABBMax(cascadePointsWS[i]);

        glm::vec3 center = (aabbMin + aabbMax) / 2.f;

        glm::vec3 lightCameraWS = center - direction * 200.f; // todo: large value;

        glm::mat4 viewMatrix = glm::lookAt(lightCameraWS, center, glm::vec3(0.f, 0.f, 1.f));

        std::vector<glm::vec3> cascadePointsLS;
        for (auto& pointWS : cascadePointsWS[i])
        {
            cascadePointsLS.emplace_back(viewMatrix * glm::vec4(pointWS, 1.f));
        }

        glm::vec3 aabbminLS = computeAABBMin(cascadePointsLS);
        glm::vec3 aabbmaxLS = computeAABBMax(cascadePointsLS);

        glm::mat4 projMatrix = glm::ortho(aabbminLS.x, aabbmaxLS.x, aabbminLS.y, aabbmaxLS.y,
            -aabbmaxLS.z, - aabbminLS.z);

        projMatrix[1][1] *= -1.f;

        m_lightProjViews[i] = projMatrix * viewMatrix;
    }
}

void icpShadowManager::UpdateCSMCB(uint32_t cascadeIndex, uint32_t curFrame)
{
    void* data;
    vmaMapMemory(m_pDevice->GetVmaAllocator(), m_csmCBAllocations[curFrame], &data);
    memcpy(data, &m_lightProjViews[cascadeIndex], sizeof(glm::mat4));
    vmaUnmapMemory(m_pDevice->GetVmaAllocator(), m_csmCBAllocations[curFrame]);
}

void icpShadowManager::UpdateCascadeShadowMapCB(uint32_t curFrame)
{
    UBOCSM ubo{};

    for (uint32_t index = 0; index < s_csmCascadeCount; index++)
    {
        ubo.CSMSplits[index] = m_cascadeSplits[index];
        ubo.CSMLightProjViewMat[index] = m_lightProjViews[index];
    }

    void* data;
    vmaMapMemory(m_pDevice->GetVmaAllocator(), m_cascadeShadowMapCBAllocations[curFrame], &data);
    memcpy(data, &ubo, sizeof(UBOCSM));
    vmaUnmapMemory(m_pDevice->GetVmaAllocator(), m_cascadeShadowMapCBAllocations[curFrame]);
}

void icpShadowManager::CreateCSMCB()
{
    VkSharingMode mode = m_pDevice->GetQueueFamilyIndices().m_graphicsFamily.value() == m_pDevice->GetQueueFamilyIndices().m_transferFamily.value() ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;

    m_csmCBs.resize(MAX_FRAMES_IN_FLIGHT);
    m_csmCBAllocations.resize(MAX_FRAMES_IN_FLIGHT);

    auto allocator = m_pDevice->GetVmaAllocator();
    auto& queueIndices = m_pDevice->GetQueueFamilyIndicesVector();

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        icpVulkanUtility::CreateGPUBuffer(
            sizeof(glm::mat4),
            mode,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            allocator,
            m_csmCBAllocations[i],
            m_csmCBs[i],
            queueIndices.size(),
            queueIndices.data()
        );
    }

    m_cascadeShadowMapCBs.resize(MAX_FRAMES_IN_FLIGHT);
    m_cascadeShadowMapCBAllocations.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        icpVulkanUtility::CreateGPUBuffer(
            sizeof(UBOCSM),
            mode,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            allocator,
            m_cascadeShadowMapCBAllocations[i],
            m_cascadeShadowMapCBs[i],
            queueIndices.size(),
            queueIndices.data()
        );
    }

}

void icpShadowManager::CreateCSMDSLayout()
{
    VkDescriptorSetLayoutBinding CSMUBOBinding{};
    CSMUBOBinding.binding = 0;
    CSMUBOBinding.descriptorCount = 1;
    CSMUBOBinding.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    CSMUBOBinding.pImmutableSamplers = nullptr;
    CSMUBOBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    m_csmDSLayout.bindings.push_back({ VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER });

    std::array<VkDescriptorSetLayoutBinding, 1> bindings{ CSMUBOBinding };

    VkDescriptorSetLayoutCreateInfo createInfo{};
    createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    createInfo.pBindings = bindings.data();
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

    if (vkCreateDescriptorSetLayout(m_pDevice->GetLogicalDevice(), &createInfo, nullptr, &m_csmDSLayout.layout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create descriptor set layout!");
    }

}

void icpShadowManager::AllocateCSMDS()
{
    icpDescriptorSetCreation creation{};
    creation.layoutInfo = m_csmDSLayout;

    std::vector<icpBufferRenderResourceInfo> bufferInfos;
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        icpBufferRenderResourceInfo bufferInfo{};
        bufferInfo.buffer = m_csmCBs[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(glm::mat4);
        bufferInfos.push_back(bufferInfo);
    }

    creation.SetUniformBuffer(0, bufferInfos);
    m_pDevice->CreateDescriptorSet(creation, m_csmDSs);
}

INCEPTION_END_NAMESPACE
