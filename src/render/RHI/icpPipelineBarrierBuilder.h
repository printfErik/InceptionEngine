#pragma once

#include "../../core/icpMacros.h"
#include "Vulkan/icpVkGPUDevice.h"
#include <vulkan/vulkan.hpp>
#include "Vulkan/icpVulkanUtility.h"


INCEPTION_BEGIN_NAMESPACE

class PipelineBarrierBuilder
{
public:

    PipelineBarrierBuilder(std::shared_ptr<icpGPUDevice> device)
        : device(device)
    {
    }

    PipelineBarrierBuilder& SetMemoryBarriers(const std::vector<VkMemoryBarrier2>& memoryBarriers);

    PipelineBarrierBuilder& SetBufferBarriers(const std::vector<VkBufferMemoryBarrier2>& bufferBarriers);

    PipelineBarrierBuilder& SetImageBarriers(const std::vector<VkImageMemoryBarrier2>& imageBarriers);
    
    VkDependencyInfo Build();

private:
    std::shared_ptr<icpGPUDevice> device;

    std::vector<VkMemoryBarrier2> m_memoryBarriers{};
    std::vector<VkBufferMemoryBarrier2> m_bufferBarriers{};
    std::vector<VkImageMemoryBarrier2> m_imageBarriers{};

    VkDependencyInfo dependencyInfo{};
};
INCEPTION_END_NAMESPACE