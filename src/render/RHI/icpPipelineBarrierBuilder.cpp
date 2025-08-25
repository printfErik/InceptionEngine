#include "icpPipelineBarrierBuilder.h"

INCEPTION_BEGIN_NAMESPACE

VkDependencyInfo PipelineBarrierBuilder::Build()
{
	dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	dependencyInfo.pNext = nullptr;

	if (!m_memoryBarriers.empty())
	{
		dependencyInfo.memoryBarrierCount = static_cast<uint32_t>(m_memoryBarriers.size());
		dependencyInfo.pMemoryBarriers = m_memoryBarriers.data();
	}
	if (!m_bufferBarriers.empty())
	{
		dependencyInfo.bufferMemoryBarrierCount = static_cast<uint32_t>(m_bufferBarriers.size());
		dependencyInfo.pBufferMemoryBarriers = m_bufferBarriers.data();
	}
	if (!m_imageBarriers.empty())
	{
		dependencyInfo.imageMemoryBarrierCount = static_cast<uint32_t>(m_imageBarriers.size());
		dependencyInfo.pImageMemoryBarriers = m_imageBarriers.data();
	}

	return dependencyInfo;
}

PipelineBarrierBuilder& PipelineBarrierBuilder::SetMemoryBarriers(const std::vector<VkMemoryBarrier2>& memoryBarriers)
{
	m_memoryBarriers = memoryBarriers;
	return *this;
}

PipelineBarrierBuilder& PipelineBarrierBuilder::SetBufferBarriers(const std::vector<VkBufferMemoryBarrier2>& bufferBarriers)
{
	m_bufferBarriers = bufferBarriers;
	return *this;
}

PipelineBarrierBuilder& PipelineBarrierBuilder::SetImageBarriers(const std::vector<VkImageMemoryBarrier2>& imageBarriers)
{
	m_imageBarriers = imageBarriers;
	return *this;
}


INCEPTION_END_NAMESPACE