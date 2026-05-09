#define VMA_IMPLEMENTATION

#include "icpVkGPUDevice.h"
#include "../../../core/icpSystemContainer.h"
#include "../../../resource/icpResourceSystem.h"
#include "../../../mesh/icpMeshResource.h"
#include "../../icpCameraSystem.h"
#include "icpVulkanUtility.h"
#include "../../icpImageResource.h"
#include "../../../core/icpLogSystem.h"

#include <iostream>
#include <map>
#include <set>
#include <array>
#include <limits>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <sstream>
#include <stdexcept>

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <imgui.h>

#include "../icpGPUBuffer.h"
#include "../../../core/icpConfigSystem.h"
#include "../../material/icpTextureRenderResourceManager.h"


INCEPTION_BEGIN_NAMESPACE

namespace
{
static uint64_t Align256(uint64_t value)
{
	return (value + 255ull) & ~255ull;
}

class icpVkBuffer : public icpRHIBuffer
{
public:
	icpVkBuffer(VkDevice device, VmaAllocator allocator, VkBuffer buffer, VmaAllocation allocation, uint64_t size, bool persistentlyMapped)
		: m_device(device)
		, m_allocator(allocator)
		, m_buffer(buffer)
		, m_allocation(allocation)
		, m_size(size)
		, m_persistentlyMapped(persistentlyMapped)
	{
	}

	~icpVkBuffer() override
	{
		if (m_buffer != VK_NULL_HANDLE)
		{
			vmaDestroyBuffer(m_allocator, m_buffer, m_allocation);
		}
	}

	void* Map() override
	{
		if (!m_mapped)
		{
			if (vmaMapMemory(m_allocator, m_allocation, &m_mapped) != VK_SUCCESS)
			{
				return nullptr;
			}
		}
		return m_mapped;
	}

	void Unmap() override
	{
		if (m_mapped)
		{
			vmaFlushAllocation(m_allocator, m_allocation, 0, VK_WHOLE_SIZE);
			if (!m_persistentlyMapped)
			{
				vmaUnmapMemory(m_allocator, m_allocation);
				m_mapped = nullptr;
			}
		}
	}

	uint64_t GetGPUAddress() const override { return 0; }
	uint64_t GetSize() const override { return m_size; }

	VkDevice m_device = VK_NULL_HANDLE;
	VmaAllocator m_allocator = VK_NULL_HANDLE;
	VkBuffer m_buffer = VK_NULL_HANDLE;
	VmaAllocation m_allocation = nullptr;
	void* m_mapped = nullptr;
	uint64_t m_size = 0;
	bool m_persistentlyMapped = false;
};

class icpVkTexture : public icpRHITexture
{
public:
	~icpVkTexture() override
	{
		if (m_view != VK_NULL_HANDLE)
		{
			vkDestroyImageView(m_device, m_view, nullptr);
		}
		if (m_image != VK_NULL_HANDLE && m_ownsImage)
		{
			vmaDestroyImage(m_allocator, m_image, m_allocation);
		}
	}

	VkDevice m_device = VK_NULL_HANDLE;
	VmaAllocator m_allocator = VK_NULL_HANDLE;
	VkImage m_image = VK_NULL_HANDLE;
	VmaAllocation m_allocation = nullptr;
	VkImageView m_view = VK_NULL_HANDLE;
	VkFormat m_vkFormat = VK_FORMAT_UNDEFINED;
	VkImageLayout m_layout = VK_IMAGE_LAYOUT_UNDEFINED;
	VkImageAspectFlags m_aspect = VK_IMAGE_ASPECT_COLOR_BIT;
	VkImageUsageFlags m_usage = 0;
	uint32_t m_mipLevels = 1;
	uint32_t m_arraySize = 1;
	bool m_ownsImage = true;
};

class icpVkSampler : public icpRHISampler
{
public:
	~icpVkSampler() override
	{
		if (m_sampler != VK_NULL_HANDLE)
		{
			vkDestroySampler(m_device, m_sampler, nullptr);
		}
	}

	VkDevice m_device = VK_NULL_HANDLE;
	VkSampler m_sampler = VK_NULL_HANDLE;
};

class icpVkPipeline : public icpRHIPipeline
{
public:
	~icpVkPipeline() override
	{
		if (m_pipeline != VK_NULL_HANDLE)
		{
			vkDestroyPipeline(m_device, m_pipeline, nullptr);
		}
		if (m_layout != VK_NULL_HANDLE)
		{
			vkDestroyPipelineLayout(m_device, m_layout, nullptr);
		}
	}

	VkDevice m_device = VK_NULL_HANDLE;
	VkPipeline m_pipeline = VK_NULL_HANDLE;
	VkPipelineLayout m_layout = VK_NULL_HANDLE;
	icpPipelineKind m_kind = icpPipelineKind::GBUFFER;
	VkPipelineBindPoint m_bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
};

class icpVkBindingSet : public icpRHIBindingSet
{
public:
	~icpVkBindingSet() override
	{
		for (VkSampler sampler : m_ownedSamplers)
		{
			if (sampler != VK_NULL_HANDLE)
			{
				vkDestroySampler(m_device, sampler, nullptr);
			}
		}
	}

	VkDevice m_device = VK_NULL_HANDLE;
	VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;
	VkDescriptorSetLayout m_layout = VK_NULL_HANDLE;
	std::vector<VkSampler> m_ownedSamplers;
	uint32_t m_resourceCount = 0;
};

class icpVkCommandList : public icpRHICommandList
{
public:
	explicit icpVkCommandList(icpQueueType queueType, VkCommandBuffer commandBuffer = VK_NULL_HANDLE)
		: m_queueType(queueType)
		, m_commandBuffer(commandBuffer)
	{
	}

	icpQueueType GetQueueType() const override { return m_queueType; }
	VkCommandBuffer GetCommandBuffer() const { return m_commandBuffer; }

private:
	icpQueueType m_queueType = icpQueueType::GRAPHICS;
	VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;
};

static VkCommandBuffer NativeCommandList(const std::shared_ptr<icpRHICommandList>& commandList)
{
	auto vkCommandList = std::dynamic_pointer_cast<icpVkCommandList>(commandList);
	return vkCommandList ? vkCommandList->GetCommandBuffer() : VK_NULL_HANDLE;
}

static icpVkBuffer* VkBufferCast(const std::shared_ptr<icpRHIBuffer>& buffer)
{
	return static_cast<icpVkBuffer*>(buffer.get());
}

static icpVkTexture* VkTextureCast(const std::shared_ptr<icpRHITexture>& texture)
{
	return static_cast<icpVkTexture*>(texture.get());
}

static icpVkPipeline* VkPipelineCast(const std::shared_ptr<icpRHIPipeline>& pipeline)
{
	return static_cast<icpVkPipeline*>(pipeline.get());
}

static icpVkBindingSet* VkBindingSetCast(const std::shared_ptr<icpRHIBindingSet>& bindingSet)
{
	return static_cast<icpVkBindingSet*>(bindingSet.get());
}

static VkPipelineStageFlags PipelineStageForState(icpResourceState state)
{
	switch (state)
	{
	case icpResourceState::COPY_DEST:
		return VK_PIPELINE_STAGE_TRANSFER_BIT;
	case icpResourceState::RENDER_TARGET:
		return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	case icpResourceState::DEPTH_WRITE:
	case icpResourceState::DEPTH_READ:
		return VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	case icpResourceState::SHADER_RESOURCE:
		return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	case icpResourceState::NON_PIXEL_SHADER_RESOURCE:
	case icpResourceState::UNORDERED_ACCESS:
		return VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	case icpResourceState::PRESENT:
		return VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	default:
		return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	}
}

static VkAccessFlags AccessMaskForState(icpResourceState state)
{
	switch (state)
	{
	case icpResourceState::COPY_DEST:
		return VK_ACCESS_TRANSFER_WRITE_BIT;
	case icpResourceState::RENDER_TARGET:
		return VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	case icpResourceState::DEPTH_WRITE:
		return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	case icpResourceState::DEPTH_READ:
		return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
	case icpResourceState::SHADER_RESOURCE:
	case icpResourceState::NON_PIXEL_SHADER_RESOURCE:
		return VK_ACCESS_SHADER_READ_BIT;
	case icpResourceState::UNORDERED_ACCESS:
		return VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
	default:
		return 0;
	}
}

static VkImageLayout ImageLayoutForState(icpResourceState state, bool depth)
{
	switch (state)
	{
	case icpResourceState::COPY_DEST:
		return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	case icpResourceState::RENDER_TARGET:
		return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	case icpResourceState::DEPTH_WRITE:
		return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	case icpResourceState::DEPTH_READ:
		return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
	case icpResourceState::SHADER_RESOURCE:
	case icpResourceState::NON_PIXEL_SHADER_RESOURCE:
		return depth ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	case icpResourceState::UNORDERED_ACCESS:
		return VK_IMAGE_LAYOUT_GENERAL;
	case icpResourceState::PRESENT:
		return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	default:
		return VK_IMAGE_LAYOUT_UNDEFINED;
	}
}

static VkImageLayout DescriptorImageLayoutForResource(icpRHIResourceViewType viewType, icpFormat format)
{
	if (viewType == icpRHIResourceViewType::UAV)
	{
		return VK_IMAGE_LAYOUT_GENERAL;
	}
	return (format == icpFormat::D32_FLOAT || format == icpFormat::D32_FLOAT_SRV) ?
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL :
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

static VkFormat ToVkFormat(icpFormat format)
{
	switch (format)
	{
	case icpFormat::R8G8B8A8_UNORM:
		return VK_FORMAT_R8G8B8A8_UNORM;
	case icpFormat::R8G8B8A8_UNORM_SRGB:
		return VK_FORMAT_R8G8B8A8_SRGB;
	case icpFormat::R32G32_FLOAT:
		return VK_FORMAT_R32G32_SFLOAT;
	case icpFormat::R32G32B32_FLOAT:
		return VK_FORMAT_R32G32B32_SFLOAT;
	case icpFormat::R16G16B16A16_FLOAT:
		return VK_FORMAT_R16G16B16A16_SFLOAT;
	case icpFormat::R32_FLOAT:
		return VK_FORMAT_R32_SFLOAT;
	case icpFormat::R8_UNORM:
		return VK_FORMAT_R8_UNORM;
	case icpFormat::D32_FLOAT:
	case icpFormat::D32_FLOAT_SRV:
		return VK_FORMAT_D32_SFLOAT;
	default:
		return VK_FORMAT_UNDEFINED;
	}
}

static VkDescriptorType DescriptorTypeForBindingResource(icpRHIResourceViewType viewType)
{
	return viewType == icpRHIResourceViewType::UAV ?
		VK_DESCRIPTOR_TYPE_STORAGE_IMAGE :
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
}

static VkImageAspectFlags AspectForFormat(icpFormat format)
{
	return format == icpFormat::D32_FLOAT || format == icpFormat::D32_FLOAT_SRV ?
		VK_IMAGE_ASPECT_DEPTH_BIT :
		VK_IMAGE_ASPECT_COLOR_BIT;
}

static bool IsDepthFormat(icpFormat format)
{
	return format == icpFormat::D32_FLOAT || format == icpFormat::D32_FLOAT_SRV;
}

static VkPrimitiveTopology ToVkTopology(icpPrimitiveTopology topology)
{
	return topology == icpPrimitiveTopology::TRIANGLE_STRIP ?
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP :
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
}

static VkCullModeFlags ToVkCullMode(icpCullMode cullMode)
{
	switch (cullMode)
	{
	case icpCullMode::NONE:
		return VK_CULL_MODE_NONE;
	case icpCullMode::FRONT:
		return VK_CULL_MODE_FRONT_BIT;
	default:
		return VK_CULL_MODE_BACK_BIT;
	}
}

static VkCompareOp ToVkCompareOp(icpCompareOp compareOp)
{
	return compareOp == icpCompareOp::ALWAYS ? VK_COMPARE_OP_ALWAYS : VK_COMPARE_OP_LESS;
}

static std::filesystem::path ShaderPathForVulkan(const std::filesystem::path& requestedPath)
{
	const std::string stem = requestedPath.stem().string();
	const std::filesystem::path shaderDir = std::filesystem::current_path() / "shaders" / "spv";
	if (stem == "CSMVS") return shaderDir / "CSM.vert.spv";
	if (stem == "CSMPS") return {};
	if (stem == "GBufferVS") return shaderDir / "GBuffer.vert.spv";
	if (stem == "GBufferPS") return shaderDir / "GBuffer.frag.spv";
	if (stem == "DeferredCompositeVS") return shaderDir / "DeferredComposite.vert.spv";
	if (stem == "DeferredCompositePS") return shaderDir / "DeferredComposite.frag.spv";
	if (stem == "TranslucentVS") return shaderDir / "Translucent.vert.spv";
	if (stem == "TranslucentPS") return shaderDir / "Translucent.frag.spv";
	if (stem == "GTAOCS") return shaderDir / "GTAO.comp.spv";
	return requestedPath;
}

static std::string FormatKey(VkFormat format)
{
	return std::to_string(static_cast<uint32_t>(format));
}

static std::string RenderTargetKey(
	const std::vector<VkFormat>& colorFormats,
	VkFormat depthFormat,
	icpRHIDepthAccess depthAccess)
{
	std::ostringstream key;
	key << "rp:";
	for (VkFormat format : colorFormats)
	{
		key << FormatKey(format) << ",";
	}
	key << "|d:" << FormatKey(depthFormat) << "|da:" << static_cast<uint32_t>(depthAccess);
	return key.str();
}

static std::string DescriptorLayoutKey(const std::vector<VkDescriptorType>& types)
{
	std::ostringstream key;
	key << "dsl:";
	for (VkDescriptorType type : types)
	{
		key << static_cast<uint32_t>(type) << ",";
	}
	return key.str();
}
}

	icpVkGPUDevice::~icpVkGPUDevice()
{
	cleanup();
}

bool icpVkGPUDevice::Initialize(std::shared_ptr<icpWindowSystem> window_system)
{
	m_window = window_system->getWindow();

	createInstance();
	initializeDebugMessenger();
	createWindowSurface();
	initializePhysicalDevice();
	createLogicalDevice();
	createVmaAllocator();
	CreateSwapChain();
	CreateSwapChainImageViews();

	createCommandPools();
	FindDepthFormat();
	CreateDepthResources();

	createDescriptorPools();
	createFence();

	return true;
}

void icpVkGPUDevice::WaitIdle()
{
	vkDeviceWaitIdle(m_device);
}

void icpVkGPUDevice::BeginFrame()
{
	WaitForFence(m_currentFrame);
	if (m_currentFrame < m_frameDescriptorPools.size() && m_frameDescriptorPools[m_currentFrame] != VK_NULL_HANDLE)
	{
		vkResetDescriptorPool(m_device, m_frameDescriptorPools[m_currentFrame], 0);
	}

	VkResult acquireResult = VK_SUCCESS;
	m_acquiredImageIndex = AcquireNextImageFromSwapchain(m_currentFrame, acquireResult);
	if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR)
	{
		ResizeSwapchain();
		return;
	}
	if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR)
	{
		throw std::runtime_error("failed to acquire swap chain image");
	}
	m_hasAcquiredImage = true;
	vkResetFences(m_device, 1, &m_inFlightFences[m_currentFrame]);

	VkCommandBuffer commandBuffer = m_graphicsCommandBuffers[m_currentFrame];
	vkResetCommandBuffer(commandBuffer, 0);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to begin graphics command buffer");
	}
}

void icpVkGPUDevice::EndFrame()
{
	if (!m_hasAcquiredImage)
	{
		return;
	}

	VkCommandBuffer commandBuffer = m_graphicsCommandBuffers[m_currentFrame];
	EndActiveRenderPass(commandBuffer);
	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to record graphics command buffer");
	}

	VkSemaphore waitSemaphores[] = { m_imageAvailableForRenderingSemaphores[m_currentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSemaphore signalSemaphores[] = { m_renderFinishedForPresentationSemaphores[m_currentFrame] };

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	if (vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inFlightFences[m_currentFrame]) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to submit graphics command buffer");
	}

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &m_swapChain;
	presentInfo.pImageIndices = &m_acquiredImageIndex;

	VkResult presentResult = vkQueuePresentKHR(m_presentQueue, &presentInfo);
	if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR || m_framebufferResized)
	{
		m_framebufferResized = false;
		ResizeSwapchain();
	}
	else if (presentResult != VK_SUCCESS)
	{
		throw std::runtime_error("failed to present swap chain image");
	}

	m_hasAcquiredImage = false;
	m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void icpVkGPUDevice::ResizeSwapchain()
{
	WaitIdle();
	CleanUpImGuiFramebuffers();
	for (auto& [_, framebuffer] : m_framebufferCache)
	{
		vkDestroyFramebuffer(m_device, framebuffer, nullptr);
	}
	m_framebufferCache.clear();
	CleanUpSwapChain();
	CreateSwapChain();
	CreateSwapChainImageViews();
	CreateDepthResources();
	CreateImGuiFramebuffers();
}

std::shared_ptr<icpRHIBuffer> icpVkGPUDevice::CreateBuffer(const icpRHIBufferDesc& desc, const void* initialData)
{
	const uint64_t size = HasUsage(desc.usage, icpBufferUsage::UNIFORM) ? Align256(desc.size) : desc.size;

	VkBufferUsageFlags usage = 0;
	if (HasUsage(desc.usage, icpBufferUsage::VERTEX)) usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	if (HasUsage(desc.usage, icpBufferUsage::INDEX)) usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	if (HasUsage(desc.usage, icpBufferUsage::UNIFORM)) usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	if (HasUsage(desc.usage, icpBufferUsage::UPLOAD)) usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	if (usage == 0) usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo allocationInfo{};
	allocationInfo.usage = VMA_MEMORY_USAGE_AUTO;
	allocationInfo.flags =
		VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT |
		VMA_ALLOCATION_CREATE_MAPPED_BIT;

	VkBuffer nativeBuffer = VK_NULL_HANDLE;
	VmaAllocation allocation = nullptr;
	VmaAllocationInfo allocationResult{};
	if (vmaCreateBuffer(m_vmaAllocator, &bufferInfo, &allocationInfo, &nativeBuffer, &allocation, &allocationResult) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create Vulkan buffer");
	}

	auto buffer = std::make_shared<icpVkBuffer>(m_device, m_vmaAllocator, nativeBuffer, allocation, size, true);
	buffer->m_mapped = allocationResult.pMappedData;
	if (initialData && desc.size > 0)
	{
		void* data = buffer->Map();
		std::memcpy(data, initialData, static_cast<size_t>(desc.size));
		vmaFlushAllocation(m_vmaAllocator, allocation, 0, VK_WHOLE_SIZE);
	}
	return buffer;
}

std::shared_ptr<icpRHITexture> icpVkGPUDevice::CreateTexture(const icpRHITextureDesc& desc, const void* initialData, size_t initialDataSize)
{
	auto texture = std::make_shared<icpVkTexture>();
	texture->m_device = m_device;
	texture->m_allocator = m_vmaAllocator;
	texture->m_format = desc.format;
	texture->m_width = desc.width;
	texture->m_height = desc.height;
	texture->m_mipLevels = desc.mipLevels;
	texture->m_arraySize = desc.arraySize;
	texture->m_vkFormat = ToVkFormat(desc.format);
	texture->m_aspect = AspectForFormat(desc.format);
	texture->m_state = initialData ? icpResourceState::COPY_DEST : desc.initialState;
	texture->m_layout = initialData ? VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL : ImageLayoutForState(desc.initialState, IsDepthFormat(desc.format));

	VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	if (HasUsage(desc.usage, icpTextureUsage::SAMPLED)) usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
	if (HasUsage(desc.usage, icpTextureUsage::RENDER_TARGET)) usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	if (HasUsage(desc.usage, icpTextureUsage::DEPTH_STENCIL)) usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	if (HasUsage(desc.usage, icpTextureUsage::STORAGE)) usage |= VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	texture->m_usage = usage;

	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent = { desc.width, desc.height, 1 };
	imageInfo.mipLevels = desc.mipLevels;
	imageInfo.arrayLayers = desc.arraySize;
	imageInfo.format = texture->m_vkFormat;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo allocationInfo{};
	allocationInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
	if (vmaCreateImage(m_vmaAllocator, &imageInfo, &allocationInfo, &texture->m_image, &texture->m_allocation, nullptr) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create Vulkan texture");
	}

	texture->m_view = icpVulkanUtility::CreateGPUImageView(
		texture->m_image,
		desc.arraySize > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D,
		texture->m_vkFormat,
		texture->m_aspect,
		desc.mipLevels,
		0,
		desc.arraySize,
		m_device);

	VkCommandBuffer commandBuffer = icpVulkanUtility::beginSingleTimeCommands(m_graphicsCommandPool, m_device);
	VkImageMemoryBarrier initBarrier{};
	initBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	initBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	initBarrier.newLayout = initialData ? VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL : texture->m_layout;
	initBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	initBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	initBarrier.image = texture->m_image;
	initBarrier.subresourceRange.aspectMask = texture->m_aspect;
	initBarrier.subresourceRange.baseMipLevel = 0;
	initBarrier.subresourceRange.levelCount = desc.mipLevels;
	initBarrier.subresourceRange.baseArrayLayer = 0;
	initBarrier.subresourceRange.layerCount = desc.arraySize;
	initBarrier.srcAccessMask = 0;
	initBarrier.dstAccessMask = initialData ? VK_ACCESS_TRANSFER_WRITE_BIT : AccessMaskForState(desc.initialState);
	vkCmdPipelineBarrier(
		commandBuffer,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		initialData ? VK_PIPELINE_STAGE_TRANSFER_BIT : PipelineStageForState(desc.initialState),
		0,
		0,
		nullptr,
		0,
		nullptr,
		1,
		&initBarrier);

	if (initialData && initialDataSize > 0)
	{
		VkBufferCreateInfo stagingInfo{};
		stagingInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		stagingInfo.size = initialDataSize;
		stagingInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		stagingInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo stagingAllocInfo{};
		stagingAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
		stagingAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
		VkBuffer stagingBuffer = VK_NULL_HANDLE;
		VmaAllocation stagingAllocation = nullptr;
		VmaAllocationInfo stagingAllocationInfo{};
		if (vmaCreateBuffer(m_vmaAllocator, &stagingInfo, &stagingAllocInfo, &stagingBuffer, &stagingAllocation, &stagingAllocationInfo) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create Vulkan texture staging buffer");
		}
		std::memcpy(stagingAllocationInfo.pMappedData, initialData, initialDataSize);
		vmaFlushAllocation(m_vmaAllocator, stagingAllocation, 0, VK_WHOLE_SIZE);

		VkBufferImageCopy copy{};
		copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copy.imageSubresource.mipLevel = 0;
		copy.imageSubresource.baseArrayLayer = 0;
		copy.imageSubresource.layerCount = 1;
		copy.imageExtent = { desc.width, desc.height, 1 };
		vkCmdCopyBufferToImage(commandBuffer, stagingBuffer, texture->m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);

		VkImageMemoryBarrier finalBarrier{};
		finalBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		finalBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		finalBarrier.newLayout = ImageLayoutForState(desc.initialState, IsDepthFormat(desc.format));
		finalBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		finalBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		finalBarrier.image = texture->m_image;
		finalBarrier.subresourceRange = initBarrier.subresourceRange;
		finalBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		finalBarrier.dstAccessMask = AccessMaskForState(desc.initialState);
		vkCmdPipelineBarrier(
			commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			PipelineStageForState(desc.initialState),
			0,
			0,
			nullptr,
			0,
			nullptr,
			1,
			&finalBarrier);
		texture->m_layout = finalBarrier.newLayout;
		texture->m_state = desc.initialState;

		icpVulkanUtility::endSingleTimeCommandsAndSubmit(commandBuffer, m_graphicsQueue, m_graphicsCommandPool, m_device);
		vmaDestroyBuffer(m_vmaAllocator, stagingBuffer, stagingAllocation);
		return texture;
	}

	icpVulkanUtility::endSingleTimeCommandsAndSubmit(commandBuffer, m_graphicsQueue, m_graphicsCommandPool, m_device);
	return texture;
}

std::shared_ptr<icpRHISampler> icpVkGPUDevice::CreateSampler()
{
	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.maxAnisotropy = 1.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = VK_LOD_CLAMP_NONE;

	auto sampler = std::make_shared<icpVkSampler>();
	sampler->m_device = m_device;
	if (vkCreateSampler(m_device, &samplerInfo, nullptr, &sampler->m_sampler) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create Vulkan sampler");
	}
	return sampler;
}

std::shared_ptr<icpRHIPipeline> icpVkGPUDevice::CreateGraphicsPipeline(const icpGraphicsPipelineDesc& desc)
{
	auto pipeline = std::make_shared<icpVkPipeline>();
	pipeline->m_device = m_device;
	pipeline->m_kind = desc.kind;
	pipeline->m_bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

	std::vector<VkDescriptorSetLayout> setLayouts;
	if (desc.kind == icpPipelineKind::GBUFFER || desc.kind == icpPipelineKind::FORWARD_TRANSLUCENT)
	{
		setLayouts = {
			GetDescriptorSetLayout({ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER }),
			GetDescriptorSetLayout({ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER }),
			GetDescriptorSetLayout({
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER }),
			GetDescriptorSetLayout({ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER }),
		};
	}
	else if (desc.kind == icpPipelineKind::DEFERRED_COMPOSITE)
	{
		setLayouts = {
			GetDescriptorSetLayout({
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER }),
			GetDescriptorSetLayout({ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER }),
			GetDescriptorSetLayout({ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER }),
		};
	}
	else if (desc.kind == icpPipelineKind::CSM)
	{
		setLayouts = {
			GetDescriptorSetLayout({ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER }),
			GetDescriptorSetLayout({ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER }),
		};
	}
	else
	{
		throw std::runtime_error("unsupported Vulkan graphics pipeline kind");
	}

	VkPushConstantRange pushRange{};
	pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	pushRange.offset = 0;
	pushRange.size = sizeof(uint32_t);

	VkPipelineLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
	layoutInfo.pSetLayouts = setLayouts.data();
	if (desc.kind == icpPipelineKind::CSM)
	{
		layoutInfo.pushConstantRangeCount = 1;
		layoutInfo.pPushConstantRanges = &pushRange;
	}
	if (vkCreatePipelineLayout(m_device, &layoutInfo, nullptr, &pipeline->m_layout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create Vulkan pipeline layout");
	}

	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
	std::vector<VkShaderModule> shaderModules;
	const auto vsPath = ShaderPathForVulkan(desc.vertexShader);
	const auto psPath = ShaderPathForVulkan(desc.pixelShader);
	if (!vsPath.empty())
	{
		VkShaderModule vs = icpVulkanUtility::createShaderModule(vsPath.string().c_str(), m_device);
		shaderModules.push_back(vs);
		VkPipelineShaderStageCreateInfo stage{};
		stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stage.stage = VK_SHADER_STAGE_VERTEX_BIT;
		stage.module = vs;
		stage.pName = "main";
		shaderStages.push_back(stage);
	}
	if (!psPath.empty())
	{
		VkShaderModule ps = icpVulkanUtility::createShaderModule(psPath.string().c_str(), m_device);
		shaderModules.push_back(ps);
		VkPipelineShaderStageCreateInfo stage{};
		stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		stage.module = ps;
		stage.pName = "main";
		shaderStages.push_back(stage);
	}

	std::vector<VkVertexInputAttributeDescription> attributes;
	for (const auto& attr : desc.vertexAttributes)
	{
		attributes.push_back({ attr.location, 0, ToVkFormat(attr.format), attr.offset });
	}
	VkVertexInputBindingDescription binding{};
	binding.binding = 0;
	binding.stride = desc.vertexStride;
	binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	VkPipelineVertexInputStateCreateInfo vertexInput{};
	vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInput.vertexBindingDescriptionCount = desc.vertexStride > 0 ? 1u : 0u;
	vertexInput.pVertexBindingDescriptions = desc.vertexStride > 0 ? &binding : nullptr;
	vertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size());
	vertexInput.pVertexAttributeDescriptions = attributes.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = ToVkTopology(desc.topology);

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.cullMode = ToVkCullMode(desc.cullMode);
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizer.lineWidth = 1.0f;
	rasterizer.depthBiasEnable = desc.kind == icpPipelineKind::CSM ? VK_TRUE : VK_FALSE;
	rasterizer.depthBiasConstantFactor = desc.kind == icpPipelineKind::CSM ? 1.25f : 0.0f;
	rasterizer.depthBiasSlopeFactor = desc.kind == icpPipelineKind::CSM ? 1.75f : 0.0f;

	VkPipelineMultisampleStateCreateInfo multisample{};
	multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	std::vector<VkPipelineColorBlendAttachmentState> colorAttachments((std::max)(size_t(1), desc.renderTargetFormats.size()));
	for (auto& attachment : colorAttachments)
	{
		attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	}
	if (desc.blendMode == icpBlendMode::TRANSLUCENT)
	{
		auto& attachment = colorAttachments[0];
		attachment.blendEnable = VK_TRUE;
		attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		attachment.colorBlendOp = VK_BLEND_OP_ADD;
		attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		attachment.alphaBlendOp = VK_BLEND_OP_ADD;
	}

	VkPipelineColorBlendStateCreateInfo colorBlend{};
	colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlend.attachmentCount = static_cast<uint32_t>(desc.renderTargetFormats.size());
	colorBlend.pAttachments = desc.renderTargetFormats.empty() ? nullptr : colorAttachments.data();

	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = desc.depthTestEnable;
	depthStencil.depthWriteEnable = desc.depthWriteEnable;
	depthStencil.depthCompareOp = ToVkCompareOp(desc.depthCompare);

	VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates = dynamicStates;

	std::vector<VkFormat> colorFormats;
	for (icpFormat format : desc.renderTargetFormats)
	{
		if (desc.kind == icpPipelineKind::DEFERRED_COMPOSITE || desc.kind == icpPipelineKind::FORWARD_TRANSLUCENT)
		{
			colorFormats.push_back(m_swapChainImageFormat);
		}
		else
		{
			colorFormats.push_back(ToVkFormat(format));
		}
	}
	VkRenderPass renderPass = GetOrCreateRenderPass(colorFormats, ToVkFormat(desc.depthFormat), icpRHIDepthAccess::WRITE);

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
	pipelineInfo.pStages = shaderStages.data();
	pipelineInfo.pVertexInputState = &vertexInput;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisample;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlend;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = pipeline->m_layout;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;
	if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline->m_pipeline) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create Vulkan graphics pipeline");
	}

	for (VkShaderModule module : shaderModules)
	{
		vkDestroyShaderModule(m_device, module, nullptr);
	}
	return pipeline;
}

std::shared_ptr<icpRHIPipeline> icpVkGPUDevice::CreateComputePipeline(const icpComputePipelineDesc& desc)
{
	auto pipeline = std::make_shared<icpVkPipeline>();
	pipeline->m_device = m_device;
	pipeline->m_kind = desc.kind;
	pipeline->m_bindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;

	std::vector<VkDescriptorSetLayout> setLayouts = {
		GetDescriptorSetLayout({ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER }),
		GetDescriptorSetLayout({ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE }),
		GetDescriptorSetLayout({ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER }),
	};
	VkPipelineLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
	layoutInfo.pSetLayouts = setLayouts.data();
	if (vkCreatePipelineLayout(m_device, &layoutInfo, nullptr, &pipeline->m_layout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create Vulkan compute pipeline layout");
	}

	const auto csPath = ShaderPathForVulkan(desc.computeShader);
	VkShaderModule cs = icpVulkanUtility::createShaderModule(csPath.string().c_str(), m_device);
	VkPipelineShaderStageCreateInfo stage{};
	stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	stage.module = cs;
	stage.pName = "main";

	VkComputePipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineInfo.stage = stage;
	pipelineInfo.layout = pipeline->m_layout;
	if (vkCreateComputePipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline->m_pipeline) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create Vulkan compute pipeline");
	}
	vkDestroyShaderModule(m_device, cs, nullptr);
	return pipeline;
}

std::shared_ptr<icpRHIBindingSet> icpVkGPUDevice::CreateBindingSet(const icpRHIBindingSetDesc& desc)
{
	auto bindingSet = std::make_shared<icpVkBindingSet>();
	bindingSet->m_device = m_device;
	bindingSet->m_resourceCount = static_cast<uint32_t>(desc.resources.size());
	if (desc.resources.empty())
	{
		return bindingSet;
	}

	std::vector<VkDescriptorType> descriptorTypes;
	descriptorTypes.reserve(desc.resources.size());
	for (const auto& resource : desc.resources)
	{
		descriptorTypes.push_back(DescriptorTypeForBindingResource(resource.viewType));
	}
	bindingSet->m_layout = GetDescriptorSetLayout(descriptorTypes);

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &bindingSet->m_layout;
	if (vkAllocateDescriptorSets(m_device, &allocInfo, &bindingSet->m_descriptorSet) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate Vulkan binding set");
	}

	std::vector<VkDescriptorImageInfo> imageInfos(desc.resources.size());
	std::vector<VkWriteDescriptorSet> writes(desc.resources.size());
	for (size_t i = 0; i < desc.resources.size(); ++i)
	{
		auto* texture = VkTextureCast(desc.resources[i].texture);
		VkSampler sampler = VK_NULL_HANDLE;
		if (desc.resources[i].viewType == icpRHIResourceViewType::SRV)
		{
			VkSamplerCreateInfo samplerInfo{};
			samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			samplerInfo.magFilter = VK_FILTER_LINEAR;
			samplerInfo.minFilter = VK_FILTER_LINEAR;
			samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			const bool materialTextureSet = desc.debugName && std::string(desc.debugName) == "MaterialTextureSet";
			const VkSamplerAddressMode addressMode = materialTextureSet ?
				VK_SAMPLER_ADDRESS_MODE_REPEAT :
				VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			samplerInfo.addressModeU = addressMode;
			samplerInfo.addressModeV = addressMode;
			samplerInfo.addressModeW = addressMode;
			samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
			if (vkCreateSampler(m_device, &samplerInfo, nullptr, &sampler) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create Vulkan binding sampler");
			}
			bindingSet->m_ownedSamplers.push_back(sampler);
		}
		imageInfos[i].sampler = sampler;
		imageInfos[i].imageView = texture->m_view;
		imageInfos[i].imageLayout = DescriptorImageLayoutForResource(desc.resources[i].viewType, texture->m_format);

		writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[i].dstSet = bindingSet->m_descriptorSet;
		writes[i].dstBinding = static_cast<uint32_t>(i);
		writes[i].descriptorCount = 1;
		writes[i].descriptorType = descriptorTypes[i];
		writes[i].pImageInfo = &imageInfos[i];
	}
	vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
	return bindingSet;
}

bool icpVkGPUDevice::SupportsAsyncCompute() const
{
	return !m_graphicsCommandBuffers.empty();
}

std::shared_ptr<icpRHICommandList> icpVkGPUDevice::GetGraphicsCommandList()
{
	if (m_graphicsCommandBuffers.empty())
	{
		return nullptr;
	}
	return std::make_shared<icpVkCommandList>(
		icpQueueType::GRAPHICS,
		m_graphicsCommandBuffers[m_currentFrame]);
}

std::shared_ptr<icpRHICommandList> icpVkGPUDevice::BeginAsyncCompute()
{
	return std::make_shared<icpVkCommandList>(
		icpQueueType::COMPUTE,
		m_graphicsCommandBuffers.empty() ? VK_NULL_HANDLE : m_graphicsCommandBuffers[m_currentFrame]);
}

uint64_t icpVkGPUDevice::EndAsyncCompute(std::shared_ptr<icpRHICommandList> commandList)
{
	VkCommandBuffer commandBuffer = NativeCommandList(commandList);
	EndActiveRenderPass(commandBuffer);
	return 0;
}

void icpVkGPUDevice::SubmitGraphicsWorkBeforeAsyncCompute()
{
}

void icpVkGPUDevice::WaitForAsyncCompute(uint64_t fenceValue)
{
	(void)fenceValue;
}

void icpVkGPUDevice::PrepareCommandList(std::shared_ptr<icpRHICommandList> commandList)
{
	VkCommandBuffer commandBuffer = NativeCommandList(commandList);
	if (commandBuffer == VK_NULL_HANDLE)
	{
		return;
	}
}

void icpVkGPUDevice::TransitionTexture(
	std::shared_ptr<icpRHICommandList> commandList,
	std::shared_ptr<icpRHITexture> texture,
	icpResourceState newState)
{
	auto* vkTexture = VkTextureCast(texture);
	if (!vkTexture)
	{
		return;
	}

	VkCommandBuffer commandBuffer = NativeCommandList(commandList);
	if (commandBuffer == VK_NULL_HANDLE)
	{
		vkTexture->m_state = newState;
		vkTexture->m_layout = ImageLayoutForState(newState, IsDepthFormat(vkTexture->m_format));
		return;
	}

	const bool depth = IsDepthFormat(vkTexture->m_format);
	const VkImageLayout newLayout = ImageLayoutForState(newState, depth);
	if (vkTexture->m_layout == newLayout)
	{
		vkTexture->m_state = newState;
		return;
	}

	EndActiveRenderPass(commandBuffer);
	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = vkTexture->m_layout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = vkTexture->m_image;
	barrier.subresourceRange.aspectMask = vkTexture->m_aspect;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = vkTexture->m_mipLevels;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = vkTexture->m_arraySize;
	barrier.srcAccessMask = AccessMaskForState(vkTexture->m_state);
	barrier.dstAccessMask = AccessMaskForState(newState);

	vkCmdPipelineBarrier(
		commandBuffer,
		PipelineStageForState(vkTexture->m_state),
		PipelineStageForState(newState),
		0,
		0,
		nullptr,
		0,
		nullptr,
		1,
		&barrier);

	vkTexture->m_state = newState;
	vkTexture->m_layout = newLayout;
}

void icpVkGPUDevice::TransitionBackBuffer(std::shared_ptr<icpRHICommandList> commandList, icpResourceState newState)
{
	if (!m_hasAcquiredImage || m_acquiredImageIndex >= m_swapChainImages.size())
	{
		return;
	}

	VkCommandBuffer commandBuffer = NativeCommandList(commandList);
	if (commandBuffer == VK_NULL_HANDLE)
	{
		return;
	}
	EndActiveRenderPass(commandBuffer);

	const VkImageLayout oldLayout = m_swapChainImageLayouts[m_acquiredImageIndex];
	const VkImageLayout newLayout = ImageLayoutForState(newState, false);
	if (oldLayout == newLayout)
	{
		return;
	}

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = m_swapChainImages[m_acquiredImageIndex];
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = oldLayout == VK_IMAGE_LAYOUT_UNDEFINED ? 0 : AccessMaskForState(icpResourceState::RENDER_TARGET);
	barrier.dstAccessMask = AccessMaskForState(newState);

	const VkPipelineStageFlags srcStage = oldLayout == VK_IMAGE_LAYOUT_UNDEFINED ?
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT :
		PipelineStageForState(icpResourceState::RENDER_TARGET);
	const VkPipelineStageFlags dstStage = PipelineStageForState(newState);

	vkCmdPipelineBarrier(
		commandBuffer,
		srcStage,
		dstStage,
		0,
		0,
		nullptr,
		0,
		nullptr,
		1,
		&barrier);

	m_swapChainImageLayouts[m_acquiredImageIndex] = newLayout;
}

void icpVkGPUDevice::SetViewportAndScissor(std::shared_ptr<icpRHICommandList> commandList, uint32_t width, uint32_t height)
{
	VkCommandBuffer commandBuffer = NativeCommandList(commandList);
	if (commandBuffer == VK_NULL_HANDLE)
	{
		return;
	}
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(width);
	viewport.height = static_cast<float>(height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = { width, height };
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void icpVkGPUDevice::SetRenderTargets(
	std::shared_ptr<icpRHICommandList> commandList,
	const std::vector<std::shared_ptr<icpRHITexture>>& colorTargets,
	std::shared_ptr<icpRHITexture> depthTarget,
	icpRHIDepthAccess depthAccess,
	bool clearColor,
	bool clearDepth)
{
	VkCommandBuffer commandBuffer = NativeCommandList(commandList);
	if (commandBuffer == VK_NULL_HANDLE)
	{
		return;
	}
	EndActiveRenderPass(commandBuffer);

	std::vector<VkFormat> colorFormats;
	std::vector<VkImageView> attachments;
	colorFormats.reserve(colorTargets.size());
	attachments.reserve(colorTargets.size() + (depthTarget ? 1 : 0));
	for (const auto& target : colorTargets)
	{
		auto* texture = VkTextureCast(target);
		colorFormats.push_back(texture->m_vkFormat);
		attachments.push_back(texture->m_view);
	}

	VkFormat depthFormat = VK_FORMAT_UNDEFINED;
	if (depthTarget)
	{
		auto* depth = VkTextureCast(depthTarget);
		depthFormat = depth->m_vkFormat;
		attachments.push_back(depth->m_view);
	}

	VkRenderPass renderPass = GetOrCreateRenderPass(colorFormats, depthFormat, depthAccess);
	VkFramebuffer framebuffer = GetOrCreateFramebuffer(
		renderPass,
		attachments,
		colorTargets.empty() ? (depthTarget ? depthTarget->m_width : 1u) : colorTargets[0]->m_width,
		colorTargets.empty() ? (depthTarget ? depthTarget->m_height : 1u) : colorTargets[0]->m_height);

	std::vector<VkClearValue> clearValues(colorTargets.size() + (depthTarget ? 1 : 0));
	for (size_t i = 0; i < colorTargets.size(); ++i)
	{
		clearValues[i].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
	}
	if (depthTarget)
	{
		clearValues.back().depthStencil = { 1.0f, 0 };
	}

	VkRenderPassBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	beginInfo.renderPass = renderPass;
	beginInfo.framebuffer = framebuffer;
	beginInfo.renderArea.offset = { 0, 0 };
	beginInfo.renderArea.extent = {
		colorTargets.empty() ? (depthTarget ? depthTarget->m_width : 1u) : colorTargets[0]->m_width,
		colorTargets.empty() ? (depthTarget ? depthTarget->m_height : 1u) : colorTargets[0]->m_height
	};
	beginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	beginInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(commandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
	m_activeRenderPass = renderPass;
	m_activeFramebuffer = framebuffer;
	m_renderPassActive = true;

	if (clearColor && !colorTargets.empty())
	{
		std::vector<VkClearAttachment> clearAttachments;
		for (uint32_t i = 0; i < colorTargets.size(); ++i)
		{
			VkClearAttachment attachment{};
			attachment.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			attachment.colorAttachment = i;
			attachment.clearValue.color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
			clearAttachments.push_back(attachment);
		}
		VkClearRect rect{};
		rect.rect = beginInfo.renderArea;
		rect.layerCount = 1;
		vkCmdClearAttachments(commandBuffer, static_cast<uint32_t>(clearAttachments.size()), clearAttachments.data(), 1, &rect);
	}
	if (clearDepth && depthTarget)
	{
		VkClearAttachment attachment{};
		attachment.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		attachment.clearValue.depthStencil = { 1.0f, 0 };
		VkClearRect rect{};
		rect.rect = beginInfo.renderArea;
		rect.layerCount = 1;
		vkCmdClearAttachments(commandBuffer, 1, &attachment, 1, &rect);
	}
}

void icpVkGPUDevice::SetBackBufferRenderTarget(std::shared_ptr<icpRHICommandList> commandList, bool clearColor)
{
	if (!m_hasAcquiredImage)
	{
		return;
	}

	VkCommandBuffer commandBuffer = NativeCommandList(commandList);
	if (commandBuffer == VK_NULL_HANDLE)
	{
		return;
	}
	EndActiveRenderPass(commandBuffer);

	std::vector<VkFormat> colorFormats = { m_swapChainImageFormat };
	std::vector<VkImageView> attachments = { m_swapChainImageViews[m_acquiredImageIndex] };
	VkRenderPass renderPass = GetOrCreateRenderPass(colorFormats, VK_FORMAT_UNDEFINED, icpRHIDepthAccess::WRITE);
	VkFramebuffer framebuffer = GetOrCreateFramebuffer(renderPass, attachments, m_swapChainExtent.width, m_swapChainExtent.height);

	VkClearValue clearValue{};
	clearValue.color = { { 0.02f, 0.02f, 0.025f, 1.0f } };
	VkRenderPassBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	beginInfo.renderPass = renderPass;
	beginInfo.framebuffer = framebuffer;
	beginInfo.renderArea.offset = { 0, 0 };
	beginInfo.renderArea.extent = m_swapChainExtent;
	beginInfo.clearValueCount = 1;
	beginInfo.pClearValues = &clearValue;
	vkCmdBeginRenderPass(commandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
	m_activeRenderPass = renderPass;
	m_activeFramebuffer = framebuffer;
	m_renderPassActive = true;

	if (clearColor)
	{
		VkClearAttachment attachment{};
		attachment.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		attachment.colorAttachment = 0;
		attachment.clearValue = clearValue;
		VkClearRect rect{};
		rect.rect = beginInfo.renderArea;
		rect.layerCount = 1;
		vkCmdClearAttachments(commandBuffer, 1, &attachment, 1, &rect);
	}
}

void icpVkGPUDevice::SetBackBufferRenderTarget(
	std::shared_ptr<icpRHICommandList> commandList,
	std::shared_ptr<icpRHITexture> depthTarget,
	icpRHIDepthAccess depthAccess,
	bool clearColor)
{
	if (!m_hasAcquiredImage)
	{
		return;
	}
	VkCommandBuffer commandBuffer = NativeCommandList(commandList);
	if (commandBuffer == VK_NULL_HANDLE)
	{
		return;
	}
	EndActiveRenderPass(commandBuffer);

	std::vector<VkFormat> colorFormats = { m_swapChainImageFormat };
	std::vector<VkImageView> attachments = { m_swapChainImageViews[m_acquiredImageIndex] };
	VkFormat depthFormat = VK_FORMAT_UNDEFINED;
	if (depthTarget)
	{
		auto* depth = VkTextureCast(depthTarget);
		depthFormat = depth->m_vkFormat;
		attachments.push_back(depth->m_view);
	}
	VkRenderPass renderPass = GetOrCreateRenderPass(colorFormats, depthFormat, depthAccess);
	VkFramebuffer framebuffer = GetOrCreateFramebuffer(renderPass, attachments, m_swapChainExtent.width, m_swapChainExtent.height);

	VkClearValue clearValues[2]{};
	clearValues[0].color = { { 0.02f, 0.02f, 0.025f, 1.0f } };
	clearValues[1].depthStencil = { 1.0f, 0 };
	VkRenderPassBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	beginInfo.renderPass = renderPass;
	beginInfo.framebuffer = framebuffer;
	beginInfo.renderArea.offset = { 0, 0 };
	beginInfo.renderArea.extent = m_swapChainExtent;
	beginInfo.clearValueCount = depthTarget ? 2u : 1u;
	beginInfo.pClearValues = clearValues;
	vkCmdBeginRenderPass(commandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
	m_activeRenderPass = renderPass;
	m_activeFramebuffer = framebuffer;
	m_renderPassActive = true;
	if (clearColor)
	{
		VkClearAttachment attachment{};
		attachment.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		attachment.colorAttachment = 0;
		attachment.clearValue = clearValues[0];
		VkClearRect rect{};
		rect.rect = beginInfo.renderArea;
		rect.layerCount = 1;
		vkCmdClearAttachments(commandBuffer, 1, &attachment, 1, &rect);
	}
}

void icpVkGPUDevice::BindGraphicsPipeline(std::shared_ptr<icpRHICommandList> commandList, std::shared_ptr<icpRHIPipeline> pipeline)
{
	VkCommandBuffer commandBuffer = NativeCommandList(commandList);
	auto* vkPipeline = VkPipelineCast(pipeline);
	if (commandBuffer == VK_NULL_HANDLE || !vkPipeline)
	{
		return;
	}
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipeline->m_pipeline);
	m_activeGraphicsPipelineLayout = vkPipeline->m_layout;
}

void icpVkGPUDevice::BindComputePipeline(std::shared_ptr<icpRHICommandList> commandList, std::shared_ptr<icpRHIPipeline> pipeline)
{
	VkCommandBuffer commandBuffer = NativeCommandList(commandList);
	auto* vkPipeline = VkPipelineCast(pipeline);
	if (commandBuffer == VK_NULL_HANDLE || !vkPipeline)
	{
		return;
	}
	EndActiveRenderPass(commandBuffer);
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, vkPipeline->m_pipeline);
	m_activeComputePipelineLayout = vkPipeline->m_layout;
}

void icpVkGPUDevice::BindGraphicsConstantBuffer(
	std::shared_ptr<icpRHICommandList> commandList,
	uint32_t bindingIndex,
	std::shared_ptr<icpRHIBuffer> buffer)
{
	VkCommandBuffer commandBuffer = NativeCommandList(commandList);
	if (commandBuffer == VK_NULL_HANDLE || m_activeGraphicsPipelineLayout == VK_NULL_HANDLE || !buffer)
	{
		return;
	}
	if (m_currentFrame >= m_frameDescriptorPools.size() || m_frameDescriptorPools[m_currentFrame] == VK_NULL_HANDLE)
	{
		return;
	}
	auto* vkBuffer = VkBufferCast(buffer);
	VkDescriptorSetLayout layout = GetDescriptorSetLayout({ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER });
	VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_frameDescriptorPools[m_currentFrame];
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &layout;
	if (vkAllocateDescriptorSets(m_device, &allocInfo, &descriptorSet) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate Vulkan graphics constant buffer descriptor");
	}
	VkDescriptorBufferInfo bufferInfo{};
	bufferInfo.buffer = vkBuffer->m_buffer;
	bufferInfo.offset = 0;
	bufferInfo.range = vkBuffer->GetSize();
	VkWriteDescriptorSet write{};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstSet = descriptorSet;
	write.dstBinding = 0;
	write.descriptorCount = 1;
	write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	write.pBufferInfo = &bufferInfo;
	vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_activeGraphicsPipelineLayout, bindingIndex, 1, &descriptorSet, 0, nullptr);
}

void icpVkGPUDevice::BindComputeConstantBuffer(
	std::shared_ptr<icpRHICommandList> commandList,
	uint32_t bindingIndex,
	std::shared_ptr<icpRHIBuffer> buffer)
{
	VkCommandBuffer commandBuffer = NativeCommandList(commandList);
	if (commandBuffer == VK_NULL_HANDLE || m_activeComputePipelineLayout == VK_NULL_HANDLE || !buffer)
	{
		return;
	}
	if (m_currentFrame >= m_frameDescriptorPools.size() || m_frameDescriptorPools[m_currentFrame] == VK_NULL_HANDLE)
	{
		return;
	}
	auto* vkBuffer = VkBufferCast(buffer);
	VkDescriptorSetLayout layout = GetDescriptorSetLayout({ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER });
	VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_frameDescriptorPools[m_currentFrame];
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &layout;
	if (vkAllocateDescriptorSets(m_device, &allocInfo, &descriptorSet) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate Vulkan compute constant buffer descriptor");
	}
	VkDescriptorBufferInfo bufferInfo{};
	bufferInfo.buffer = vkBuffer->m_buffer;
	bufferInfo.offset = 0;
	bufferInfo.range = vkBuffer->GetSize();
	VkWriteDescriptorSet write{};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstSet = descriptorSet;
	write.dstBinding = 0;
	write.descriptorCount = 1;
	write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	write.pBufferInfo = &bufferInfo;
	vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_activeComputePipelineLayout, bindingIndex, 1, &descriptorSet, 0, nullptr);
}

void icpVkGPUDevice::BindGraphicsBindingSet(
	std::shared_ptr<icpRHICommandList> commandList,
	uint32_t bindingIndex,
	std::shared_ptr<icpRHIBindingSet> bindingSet)
{
	VkCommandBuffer commandBuffer = NativeCommandList(commandList);
	auto* vkBindingSet = VkBindingSetCast(bindingSet);
	if (commandBuffer == VK_NULL_HANDLE || m_activeGraphicsPipelineLayout == VK_NULL_HANDLE || !vkBindingSet || vkBindingSet->m_descriptorSet == VK_NULL_HANDLE)
	{
		return;
	}
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_activeGraphicsPipelineLayout, bindingIndex, 1, &vkBindingSet->m_descriptorSet, 0, nullptr);
}

void icpVkGPUDevice::BindComputeBindingSet(
	std::shared_ptr<icpRHICommandList> commandList,
	uint32_t bindingIndex,
	std::shared_ptr<icpRHIBindingSet> bindingSet)
{
	VkCommandBuffer commandBuffer = NativeCommandList(commandList);
	auto* vkBindingSet = VkBindingSetCast(bindingSet);
	if (commandBuffer == VK_NULL_HANDLE || m_activeComputePipelineLayout == VK_NULL_HANDLE || !vkBindingSet || vkBindingSet->m_descriptorSet == VK_NULL_HANDLE)
	{
		return;
	}
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_activeComputePipelineLayout, bindingIndex, 1, &vkBindingSet->m_descriptorSet, 0, nullptr);
}

void icpVkGPUDevice::SetGraphicsConstant(
	std::shared_ptr<icpRHICommandList> commandList,
	uint32_t bindingIndex,
	uint32_t value)
{
	VkCommandBuffer commandBuffer = NativeCommandList(commandList);
	if (commandBuffer == VK_NULL_HANDLE || m_activeGraphicsPipelineLayout == VK_NULL_HANDLE)
	{
		return;
	}
	(void)bindingIndex;
	vkCmdPushConstants(commandBuffer, m_activeGraphicsPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(uint32_t), &value);
}

void icpVkGPUDevice::BindVertexAndIndexBuffers(
	std::shared_ptr<icpRHICommandList> commandList,
	std::shared_ptr<icpRHIBuffer> vertexBuffer,
	uint64_t vertexBufferSize,
	std::shared_ptr<icpRHIBuffer> indexBuffer,
	uint64_t indexBufferSize,
	uint32_t vertexStride)
{
	VkCommandBuffer commandBuffer = NativeCommandList(commandList);
	if (commandBuffer == VK_NULL_HANDLE)
	{
		return;
	}
	VkBuffer vb = VkBufferCast(vertexBuffer)->m_buffer;
	VkDeviceSize offset = 0;
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vb, &offset);
	vkCmdBindIndexBuffer(commandBuffer, VkBufferCast(indexBuffer)->m_buffer, 0, VK_INDEX_TYPE_UINT32);
	(void)vertexBufferSize;
	(void)indexBufferSize;
	(void)vertexStride;
}

void icpVkGPUDevice::DrawIndexed(std::shared_ptr<icpRHICommandList> commandList, uint32_t indexCount)
{
	VkCommandBuffer commandBuffer = NativeCommandList(commandList);
	if (commandBuffer != VK_NULL_HANDLE)
	{
		vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
	}
}

void icpVkGPUDevice::Draw(std::shared_ptr<icpRHICommandList> commandList, uint32_t vertexCount)
{
	VkCommandBuffer commandBuffer = NativeCommandList(commandList);
	if (commandBuffer != VK_NULL_HANDLE)
	{
		vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
	}
}

void icpVkGPUDevice::Dispatch(
	std::shared_ptr<icpRHICommandList> commandList,
	uint32_t groupCountX,
	uint32_t groupCountY,
	uint32_t groupCountZ)
{
	VkCommandBuffer commandBuffer = NativeCommandList(commandList);
	if (commandBuffer != VK_NULL_HANDLE)
	{
		EndActiveRenderPass(commandBuffer);
		vkCmdDispatch(commandBuffer, groupCountX, groupCountY, groupCountZ);
	}
}

void icpVkGPUDevice::InitializeImGui(std::shared_ptr<icpWindowSystem> windowSystem)
{
	if (m_imguiInitialized)
	{
		return;
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	ImGui_ImplGlfw_InitForVulkan(windowSystem->getWindow(), true);

	CreateImGuiRenderPass();
	CreateImGuiFramebuffers();

	ImGui_ImplVulkan_InitInfo initInfo{};
	initInfo.Instance = m_instance;
	initInfo.PhysicalDevice = m_physicalDevice;
	initInfo.Device = m_device;
	initInfo.QueueFamily = m_queueIndices.m_graphicsFamily.value();
	initInfo.Queue = m_graphicsQueue;
	initInfo.PipelineCache = VK_NULL_HANDLE;
	initInfo.DescriptorPool = m_descriptorPool;
	initInfo.Subpass = 0;
	initInfo.MinImageCount = (std::max)(2u, static_cast<uint32_t>(m_swapChainImages.size()));
	initInfo.ImageCount = static_cast<uint32_t>(m_swapChainImages.size());
	initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	initInfo.UseDynamicRendering = false;
	initInfo.Allocator = nullptr;
	initInfo.CheckVkResultFn = nullptr;

	if (!ImGui_ImplVulkan_Init(&initInfo, m_imguiRenderPass))
	{
		throw std::runtime_error("failed to initialize Vulkan ImGui backend");
	}

	m_imguiInitialized = true;
}

void icpVkGPUDevice::ShutdownImGui()
{
	CleanUpImGui();
}

void icpVkGPUDevice::BeginImGuiFrame()
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void icpVkGPUDevice::RenderImGuiDrawData(std::shared_ptr<icpRHICommandList> commandList)
{
	if (!m_imguiInitialized || !m_hasAcquiredImage || m_acquiredImageIndex >= m_imguiFramebuffers.size())
	{
		return;
	}

	VkCommandBuffer commandBuffer = NativeCommandList(commandList);
	if (commandBuffer == VK_NULL_HANDLE)
	{
		return;
	}
	EndActiveRenderPass(commandBuffer);

	VkRenderPassBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	beginInfo.renderPass = m_imguiRenderPass;
	beginInfo.framebuffer = m_imguiFramebuffers[m_acquiredImageIndex];
	beginInfo.renderArea.offset = { 0, 0 };
	beginInfo.renderArea.extent = m_swapChainExtent;

	vkCmdBeginRenderPass(commandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
	vkCmdEndRenderPass(commandBuffer);
}

VkDescriptorSetLayout icpVkGPUDevice::GetDescriptorSetLayout(const std::vector<VkDescriptorType>& descriptorTypes)
{
	const std::string key = DescriptorLayoutKey(descriptorTypes);
	if (auto it = m_descriptorSetLayoutCache.find(key); it != m_descriptorSetLayoutCache.end())
	{
		return it->second;
	}

	std::vector<VkDescriptorSetLayoutBinding> bindings;
	bindings.reserve(descriptorTypes.size());
	for (uint32_t i = 0; i < descriptorTypes.size(); ++i)
	{
		VkDescriptorSetLayoutBinding binding{};
		binding.binding = i;
		binding.descriptorType = descriptorTypes[i];
		binding.descriptorCount = 1;
		binding.stageFlags = VK_SHADER_STAGE_ALL;
		bindings.push_back(binding);
	}

	VkDescriptorSetLayoutCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	createInfo.pBindings = bindings.data();
	VkDescriptorSetLayout layout = VK_NULL_HANDLE;
	if (vkCreateDescriptorSetLayout(m_device, &createInfo, nullptr, &layout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create Vulkan descriptor set layout");
	}
	m_descriptorSetLayoutCache[key] = layout;
	return layout;
}

VkRenderPass icpVkGPUDevice::GetOrCreateRenderPass(
	const std::vector<VkFormat>& colorFormats,
	VkFormat depthFormat,
	icpRHIDepthAccess depthAccess)
{
	const std::string key = RenderTargetKey(colorFormats, depthFormat, depthAccess);
	if (auto it = m_renderPassCache.find(key); it != m_renderPassCache.end())
	{
		return it->second;
	}

	std::vector<VkAttachmentDescription> attachments;
	std::vector<VkAttachmentReference> colorRefs;
	attachments.reserve(colorFormats.size() + (depthFormat == VK_FORMAT_UNDEFINED ? 0 : 1));
	colorRefs.reserve(colorFormats.size());
	for (uint32_t i = 0; i < colorFormats.size(); ++i)
	{
		VkAttachmentDescription attachment{};
		attachment.format = colorFormats[i];
		attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachments.push_back(attachment);

		VkAttachmentReference ref{};
		ref.attachment = i;
		ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		colorRefs.push_back(ref);
	}

	VkAttachmentReference depthRef{};
	const bool hasDepth = depthFormat != VK_FORMAT_UNDEFINED;
	if (hasDepth)
	{
		VkAttachmentDescription attachment{};
		attachment.format = depthFormat;
		attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachment.initialLayout = depthAccess == icpRHIDepthAccess::READ ?
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL :
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		attachment.finalLayout = attachment.initialLayout;
		depthRef.attachment = static_cast<uint32_t>(attachments.size());
		depthRef.layout = attachment.initialLayout;
		attachments.push_back(attachment);
	}

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = static_cast<uint32_t>(colorRefs.size());
	subpass.pColorAttachments = colorRefs.empty() ? nullptr : colorRefs.data();
	subpass.pDepthStencilAttachment = hasDepth ? &depthRef : nullptr;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstStageMask = dependency.srcStageMask;
	dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependency.dstAccessMask = dependency.srcAccessMask | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	VkRenderPass renderPass = VK_NULL_HANDLE;
	if (vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create Vulkan render pass");
	}
	m_renderPassCache[key] = renderPass;
	return renderPass;
}

VkFramebuffer icpVkGPUDevice::GetOrCreateFramebuffer(
	VkRenderPass renderPass,
	const std::vector<VkImageView>& attachments,
	uint32_t width,
	uint32_t height)
{
	std::ostringstream key;
	key << "fb:" << renderPass << ":" << width << "x" << height << ":";
	for (VkImageView view : attachments)
	{
		key << view << ",";
	}
	const std::string cacheKey = key.str();
	if (auto it = m_framebufferCache.find(cacheKey); it != m_framebufferCache.end())
	{
		return it->second;
	}

	VkFramebufferCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	createInfo.renderPass = renderPass;
	createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	createInfo.pAttachments = attachments.data();
	createInfo.width = width;
	createInfo.height = height;
	createInfo.layers = 1;
	VkFramebuffer framebuffer = VK_NULL_HANDLE;
	if (vkCreateFramebuffer(m_device, &createInfo, nullptr, &framebuffer) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create Vulkan framebuffer");
	}
	m_framebufferCache[cacheKey] = framebuffer;
	return framebuffer;
}

void icpVkGPUDevice::EndActiveRenderPass(VkCommandBuffer commandBuffer)
{
	if (m_renderPassActive && commandBuffer != VK_NULL_HANDLE)
	{
		vkCmdEndRenderPass(commandBuffer);
		m_renderPassActive = false;
		m_activeRenderPass = VK_NULL_HANDLE;
		m_activeFramebuffer = VK_NULL_HANDLE;
	}
}

void icpVkGPUDevice::CleanUpRenderCaches()
{
	for (auto& [_, framebuffer] : m_framebufferCache)
	{
		vkDestroyFramebuffer(m_device, framebuffer, nullptr);
	}
	m_framebufferCache.clear();

	for (auto& [_, renderPass] : m_renderPassCache)
	{
		vkDestroyRenderPass(m_device, renderPass, nullptr);
	}
	m_renderPassCache.clear();

	for (auto& [_, layout] : m_descriptorSetLayoutCache)
	{
		vkDestroyDescriptorSetLayout(m_device, layout, nullptr);
	}
	m_descriptorSetLayoutCache.clear();
}

void icpVkGPUDevice::CreateImGuiRenderPass()
{
	if (m_imguiRenderPass != VK_NULL_HANDLE)
	{
		return;
	}

	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = m_swapChainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_imguiRenderPass) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create ImGui render pass");
	}
}

void icpVkGPUDevice::CreateImGuiFramebuffers()
{
	CleanUpImGuiFramebuffers();
	if (m_imguiRenderPass == VK_NULL_HANDLE || m_swapChainImageViews.empty())
	{
		return;
	}

	m_imguiFramebuffers.resize(m_swapChainImageViews.size());
	for (size_t i = 0; i < m_swapChainImageViews.size(); ++i)
	{
		VkImageView attachments[] = { m_swapChainImageViews[i] };

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = m_imguiRenderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = m_swapChainExtent.width;
		framebufferInfo.height = m_swapChainExtent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &m_imguiFramebuffers[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create ImGui framebuffer");
		}
	}
}

void icpVkGPUDevice::CleanUpImGuiFramebuffers()
{
	for (VkFramebuffer framebuffer : m_imguiFramebuffers)
	{
		if (framebuffer != VK_NULL_HANDLE)
		{
			vkDestroyFramebuffer(m_device, framebuffer, nullptr);
		}
	}
	m_imguiFramebuffers.clear();
}

void icpVkGPUDevice::CleanUpImGui()
{
	if (m_imguiInitialized)
	{
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
		m_imguiInitialized = false;
	}

	CleanUpImGuiFramebuffers();
	if (m_imguiRenderPass != VK_NULL_HANDLE)
	{
		vkDestroyRenderPass(m_device, m_imguiRenderPass, nullptr);
		m_imguiRenderPass = VK_NULL_HANDLE;
	}
}

void icpVkGPUDevice::createVmaAllocator()
{
	VmaAllocatorCreateInfo vma_create_info{};
	vma_create_info.vulkanApiVersion = VK_API_VERSION_1_2;
	vma_create_info.device = m_device;
	vma_create_info.instance = m_instance;
	vma_create_info.physicalDevice = m_physicalDevice;

	VkResult result = vmaCreateAllocator(&vma_create_info, &m_vmaAllocator);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create VMA allocator");
	}
}

void icpVkGPUDevice::createInstance()
{
	if (m_enableValidationLayers && !checkValidationLayerSupport())
	{
		std::cerr << "validation layer unavailable, continuing without it" << std::endl;
		m_enableValidationLayers = false;
	}

	// app info
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;

	appInfo.apiVersion = VK_API_VERSION_1_2;
	
	appInfo.engineVersion = VK_MAKE_API_VERSION(0, 0, 1, 0);
	appInfo.applicationVersion = VK_MAKE_API_VERSION(0, 0, 1, 0);
	appInfo.pApplicationName = "Inception_Renderer";
	appInfo.pEngineName = "Inception_Engine";

	// instance create info
	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	if (m_enableValidationLayers)
	{
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};

	if (m_enableValidationLayers)
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
		createInfo.ppEnabledLayerNames = m_validationLayers.data();

		populateDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
	}

	if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS)
	{
		throw std::runtime_error("vk create instance failed");
	}
}

void icpVkGPUDevice::cleanup()
{
	if (m_device != VK_NULL_HANDLE)
	{
		vkDeviceWaitIdle(m_device);
	}

	CleanUpImGui();
	CleanUpSwapChain();
	CleanUpRenderCaches();

	for (size_t i = 0; i < m_inFlightFences.size(); i++)
	{
		if (i < m_imageAvailableForRenderingSemaphores.size())
		{
			vkDestroySemaphore(m_device, m_imageAvailableForRenderingSemaphores[i], nullptr);
		}
		if (i < m_renderFinishedForPresentationSemaphores.size())
		{
			vkDestroySemaphore(m_device, m_renderFinishedForPresentationSemaphores[i], nullptr);
		}
		vkDestroyFence(m_device, m_inFlightFences[i], nullptr);
	}

	if (m_graphicsCommandPool != VK_NULL_HANDLE)
	{
		vkDestroyCommandPool(m_device, m_graphicsCommandPool, nullptr);
	}
	if (m_transferCommandPool != VK_NULL_HANDLE)
	{
		vkDestroyCommandPool(m_device, m_transferCommandPool, nullptr);
	}
	if (m_computeCommandPool != VK_NULL_HANDLE)
	{
		vkDestroyCommandPool(m_device, m_computeCommandPool, nullptr);
	}

	if (m_descriptorPool != VK_NULL_HANDLE)
	{
		vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
		m_descriptorPool = VK_NULL_HANDLE;
	}
	for (VkDescriptorPool pool : m_frameDescriptorPools)
	{
		if (pool != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorPool(m_device, pool, nullptr);
		}
	}
	m_frameDescriptorPools.clear();

	if (m_vmaAllocator != VK_NULL_HANDLE)
	{
		vmaDestroyAllocator(m_vmaAllocator);
	}

	if (m_device != VK_NULL_HANDLE)
	{
		vkDestroyDevice(m_device, nullptr);
	}

	if (m_enableValidationLayers && m_debugMessenger != VK_NULL_HANDLE)
	{
		destroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
	}

	if (m_surface != VK_NULL_HANDLE)
	{
		vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
	}
	
	if (m_instance != VK_NULL_HANDLE)
	{
		vkDestroyInstance(m_instance, nullptr);
	}
}

void icpVkGPUDevice::CleanUpSwapChain()
{
	if (m_depthImageView != VK_NULL_HANDLE)
	{
		vkDestroyImageView(m_device, m_depthImageView, nullptr);
		m_depthImageView = VK_NULL_HANDLE;
	}
	if (m_depthImage != VK_NULL_HANDLE)
	{
		vmaDestroyImage(m_vmaAllocator, m_depthImage, m_depthBufferAllocation);
		m_depthImage = VK_NULL_HANDLE;
		m_depthBufferAllocation = nullptr;
	}

	for (const auto& imgView : m_swapChainImageViews)
	{
		vkDestroyImageView(m_device, imgView, nullptr);
	}
	m_swapChainImageViews.clear();

	if (m_swapChain != VK_NULL_HANDLE)
	{
		vkDestroySwapchainKHR(m_device, m_swapChain, nullptr);
		m_swapChain = VK_NULL_HANDLE;
	}
	m_swapChainImages.clear();
	m_swapChainImageLayouts.clear();
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT,
	VkDebugUtilsMessageTypeFlagsEXT,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
	return VK_FALSE;
}

void icpVkGPUDevice::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity =
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | 
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType =
		VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | 
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;
}

bool icpVkGPUDevice::checkValidationLayerSupport()
{
	uint32_t layersCount;
	vkEnumerateInstanceLayerProperties(&layersCount, nullptr);
	std::vector<VkLayerProperties> layerProperties(layersCount);
	vkEnumerateInstanceLayerProperties(&layersCount, layerProperties.data());

	for (const char* name : m_validationLayers)
	{
		bool layerFound = false;
		for (const auto& layerP : layerProperties)
		{
			if (std::strcmp(layerP.layerName, name) == 0)
			{
				layerFound = true;
				break;
			}
		}
		if (!layerFound)
		{
			return false;
		}
	}
	return true;
}

void icpVkGPUDevice::initializeDebugMessenger()
{
	if (m_enableValidationLayers)
	{
		VkDebugUtilsMessengerCreateInfoEXT createInfo;
		populateDebugMessengerCreateInfo(createInfo);
		auto result = createDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &m_debugMessenger);
		if (VK_SUCCESS != result)
		{
			throw std::runtime_error("failed to set up debug messenger!");
		}
	}

	if (m_enableDebugUtilsLabel)
	{
		m_vk_cmd_begin_debug_utils_label_ext =
			(PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetInstanceProcAddr(m_instance, "vkCmdBeginDebugUtilsLabelEXT");
		m_vk_cmd_end_debug_utils_label_ext =
			(PFN_vkCmdEndDebugUtilsLabelEXT)vkGetInstanceProcAddr(m_instance, "vkCmdEndDebugUtilsLabelEXT");
	}
}

VkResult icpVkGPUDevice::createDebugUtilsMessengerEXT(VkInstance instance,
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	auto func =
		(PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void icpVkGPUDevice::destroyDebugUtilsMessengerEXT(VkInstance instance,
	VkDebugUtilsMessengerEXT     debugMessenger,
	const VkAllocationCallbacks* pAllocator)
{
	auto func =
		(PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		func(instance, debugMessenger, pAllocator);
	}
}

void icpVkGPUDevice::createWindowSurface()
{
	if (glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface) != VK_SUCCESS)
	{
		throw std::runtime_error("glfwCreateWindowSurface failed");
	}
}

void icpVkGPUDevice::initializePhysicalDevice()
{
	uint32_t physicalDeviceCount = 0;
	vkEnumeratePhysicalDevices(m_instance, &physicalDeviceCount, nullptr);

	if (physicalDeviceCount == 0)
	{
		throw std::runtime_error("failed to find GPUs with Vulkan support");
	}

	std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
	vkEnumeratePhysicalDevices(m_instance, &physicalDeviceCount, physicalDevices.data());

	std::vector<std::pair<int, VkPhysicalDevice>> rankedPhysicalDevices;
	for (const auto& device : physicalDevices)
	{
		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(device, &properties);

		int score = 0;

		if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			score += 1000;
		}
		else if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
		{
			score += 100;
		}

		rankedPhysicalDevices.push_back(std::make_pair(score, device));
	}

	std::sort(rankedPhysicalDevices.begin(), rankedPhysicalDevices.end(), 
		[](const std::pair<int, VkPhysicalDevice>& p1, const std::pair<int, VkPhysicalDevice>& p2)
		{
			return p1.first > p2.first;
		});

	for (const auto& device: rankedPhysicalDevices)
	{
		if(isDeviceSuitable(device.second))
		{
			m_physicalDevice = device.second;
			break;
		}
	}

	if (m_physicalDevice == VK_NULL_HANDLE)
	{
		throw std::runtime_error("failed to find suitable physical device");
	}
}

bool icpVkGPUDevice::isDeviceSuitable(VkPhysicalDevice device)
{
	auto queueIndices = findQueueFamilies(device);
	bool isExtensionsSupported = checkDeviceExtensionSupport(device);
	bool isSwapchainAdequate = false;
	if (isExtensionsSupported)
	{
		SwapChainSupportDetails swapchainSupportDetails = querySwapChainSupport(device);
		isSwapchainAdequate =
			!swapchainSupportDetails.m_formats.empty() && !swapchainSupportDetails.m_presentModes.empty();
	}

	VkPhysicalDeviceFeatures physical_device_features;
	vkGetPhysicalDeviceFeatures(device, &physical_device_features);

	return queueIndices.isComplete() && isSwapchainAdequate && physical_device_features.samplerAnisotropy;
}

bool icpVkGPUDevice::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, extensions.data());

	std::set<std::string> requiredExtensions(m_requiredDeviceExtensions.begin(), m_requiredDeviceExtensions.end());
	for (const auto& extension : extensions)
	{
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}


QueueFamilyIndices icpVkGPUDevice::findQueueFamilies(VkPhysicalDevice device)
{
	QueueFamilyIndices indices;
	uint32_t indicesCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &indicesCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilies(indicesCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &indicesCount, queueFamilies.data());

	int i = 0;
	for (const auto& queueFamily : queueFamilies)
	{
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			indices.m_graphicsFamily = i;
		}

		if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)
		{
			indices.m_computeFamily = i;
		}

		if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT)
		{
			indices.m_transferFamily = i;
		}

		i++;
	}

	VkBool32 isPresentSupport = false;
	vkGetPhysicalDeviceSurfaceSupportKHR(device, indices.m_graphicsFamily.value(), m_surface, &isPresentSupport);
	if (isPresentSupport)
	{
		indices.m_presentFamily = indices.m_graphicsFamily.value();
	}

	return indices;
}

void icpVkGPUDevice::createLogicalDevice()
{
	m_queueIndices = findQueueFamilies(m_physicalDevice);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> queueFamilies = {
		m_queueIndices.m_graphicsFamily.value(),
		m_queueIndices.m_presentFamily.value(),
		m_queueIndices.m_transferFamily.value(),
		m_queueIndices.m_computeFamily.value()
	};

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : queueFamilies)
	{
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures phyDeviceFeatures{};
	phyDeviceFeatures.geometryShader = VK_TRUE;
	phyDeviceFeatures.independentBlend = VK_TRUE;
	phyDeviceFeatures.samplerAnisotropy = VK_TRUE;

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pEnabledFeatures = &phyDeviceFeatures;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(m_requiredDeviceExtensions.size());
	createInfo.ppEnabledExtensionNames = m_requiredDeviceExtensions.data();
	createInfo.enabledLayerCount = 0;

	if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device) != VK_SUCCESS)
	{
		throw std::runtime_error("create logical device failed");
	}

	// initialize queues of this device
	vkGetDeviceQueue(m_device, m_queueIndices.m_graphicsFamily.value(), 0, &m_graphicsQueue);
	vkGetDeviceQueue(m_device, m_queueIndices.m_presentFamily.value(), 0, &m_presentQueue);
	vkGetDeviceQueue(m_device, m_queueIndices.m_transferFamily.value(), 0, &m_transferQueue);
	vkGetDeviceQueue(m_device, m_queueIndices.m_computeFamily.value(), 0, &m_computeQueue);

	for (auto& index : queueFamilies)
	{
		m_queueFamilyIndices.push_back(index);
	}
}

SwapChainSupportDetails icpVkGPUDevice::querySwapChainSupport(VkPhysicalDevice device)
{
	SwapChainSupportDetails details_result;

	// capabilities
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &details_result.m_capabilities);

	// formats
	uint32_t format_count;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &format_count, nullptr);
	if (format_count != 0)
	{
		details_result.m_formats.resize(format_count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(
			device, m_surface, &format_count, details_result.m_formats.data());
	}

	// present modes
	uint32_t presentmode_count;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentmode_count, nullptr);
	if (presentmode_count != 0)
	{
		details_result.m_presentModes.resize(presentmode_count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(
			device, m_surface, &presentmode_count, details_result.m_presentModes.data());
	}

	return details_result;
}

VkSurfaceFormatKHR icpVkGPUDevice::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	for (const auto& format : availableFormats)
	{
		if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return format;
		}
	}
	return availableFormats[0];
}

VkPresentModeKHR icpVkGPUDevice::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
	for (const auto& mode : availablePresentModes)
	{
		if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return mode;
		}
	}
	return availablePresentModes[0];
}

VkExtent2D icpVkGPUDevice::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
	if (capabilities.currentExtent.width != UINT32_MAX)
	{
		return capabilities.currentExtent;
	}
	
	int width, height;
	glfwGetFramebufferSize(m_window, &width, &height);

	VkExtent2D actualExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

	actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
	actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
	return actualExtent;
}

void icpVkGPUDevice::CreateSwapChain()
{
	SwapChainSupportDetails swapInfo = querySwapChainSupport(m_physicalDevice);

	auto surfaceFormat = chooseSwapSurfaceFormat(swapInfo.m_formats);
	auto presentMode = chooseSwapPresentMode(swapInfo.m_presentModes);
	auto swapExtent = chooseSwapExtent(swapInfo.m_capabilities);

	uint32_t imgCount = swapInfo.m_capabilities.minImageCount + 1;

	if (swapInfo.m_capabilities.maxImageCount > 0 && swapInfo.m_capabilities.maxImageCount < imgCount)
	{
		imgCount = swapInfo.m_capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = m_surface;

	createInfo.presentMode = presentMode;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = swapExtent;
	createInfo.minImageCount = imgCount;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	uint32_t queueFamilyIndices[] = 
	{
		m_queueIndices.m_graphicsFamily.value(),
		m_queueIndices.m_transferFamily.value(),
		m_queueIndices.m_computeFamily.value()
	};

	std::set<uint32_t> queueFamilyIndexSet = 
	{
		m_queueIndices.m_graphicsFamily.value(),
		m_queueIndices.m_transferFamily.value(),
		m_queueIndices.m_computeFamily.value()
	};
	if (queueFamilyIndexSet.size() == 1)
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}
	else
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = queueFamilyIndexSet.size();
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}

	createInfo.preTransform = swapInfo.m_capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.clipped = VK_TRUE;

	createInfo.oldSwapchain = VK_NULL_HANDLE;

	auto result = vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapChain);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("create swap chain failed");
	}

	vkGetSwapchainImagesKHR(m_device, m_swapChain, &imgCount, nullptr);
	m_swapChainImages.resize(imgCount);
	vkGetSwapchainImagesKHR(m_device, m_swapChain, &imgCount, m_swapChainImages.data());

	m_swapChainImageFormat = surfaceFormat.format;
	m_swapChainExtent = swapExtent;
	m_swapChainImageLayouts.assign(m_swapChainImages.size(), VK_IMAGE_LAYOUT_UNDEFINED);
}

void icpVkGPUDevice::CreateSwapChainImageViews()
{
	m_swapChainImageViews.resize(m_swapChainImages.size());

	for (size_t i = 0; i < m_swapChainImages.size(); i++) 
	{
		m_swapChainImageViews[i] = icpVulkanUtility::CreateGPUImageView(
			m_swapChainImages[i], 
			VK_IMAGE_VIEW_TYPE_2D,
			m_swapChainImageFormat, 
			VK_IMAGE_ASPECT_COLOR_BIT, 
			1,0, 1,
			m_device
		);
	}
}

void icpVkGPUDevice::createCommandPools()
{
	VkCommandPoolCreateInfo gCreateInfo{};
	gCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	gCreateInfo.flags = VkCommandPoolCreateFlagBits::VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	gCreateInfo.queueFamilyIndex = m_queueIndices.m_graphicsFamily.value();

	if (vkCreateCommandPool(m_device, &gCreateInfo, nullptr, &m_graphicsCommandPool))
	{
		throw std::runtime_error("failed to create graphics command pool!");
	}

	VkCommandPoolCreateInfo tCreateInfo{};
	tCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	tCreateInfo.flags = VkCommandPoolCreateFlagBits::VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	tCreateInfo.queueFamilyIndex = m_queueIndices.m_transferFamily.value();

	if (vkCreateCommandPool(m_device, &tCreateInfo, nullptr, &m_transferCommandPool))
	{
		throw std::runtime_error("failed to create command pool!");
	}

	VkCommandPoolCreateInfo computeCreateInfo = {};
	computeCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	computeCreateInfo.queueFamilyIndex = m_queueIndices.m_computeFamily.value();
	computeCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	if (vkCreateCommandPool(m_device, &computeCreateInfo, nullptr, &m_computeCommandPool) != VK_SUCCESS) {
		throw std::runtime_error("Could not create compute command pool");
	}

	m_graphicsCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_graphicsCommandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = static_cast<uint32_t>(m_graphicsCommandBuffers.size());
	if (vkAllocateCommandBuffers(m_device, &allocInfo, m_graphicsCommandBuffers.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate graphics command buffers");
	}
}

void icpVkGPUDevice::FindDepthFormat()
{
	m_depthFormat = icpVulkanUtility::findDepthFormat(m_physicalDevice);
}

void icpVkGPUDevice::CreateDepthResources() {
	

	icpVulkanUtility::CreateGPUImage(
		m_swapChainExtent.width, 
		m_swapChainExtent.height,
		1,
		1,
		m_depthFormat,
		VK_IMAGE_TILING_OPTIMAL, 
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
		m_vmaAllocator,
		m_depthImage, 
		m_depthBufferAllocation
	);
	m_depthImageView = icpVulkanUtility::CreateGPUImageView(
		m_depthImage, 
		VK_IMAGE_VIEW_TYPE_2D, 
		m_depthFormat, 
		VK_IMAGE_ASPECT_DEPTH_BIT, 
		1,0, 1, m_device
	);
}

bool hasStencilComponent(VkFormat format) {
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void icpVkGPUDevice::createDescriptorPools()
{
	std::array<VkDescriptorPoolSize, 4> poolSize{};
	poolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSize[0].descriptorCount = 512;
	poolSize[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSize[1].descriptorCount = 4096;
	poolSize[2].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	poolSize[2].descriptorCount = 512;
	poolSize[3].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	poolSize[3].descriptorCount = 512;

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	poolInfo.poolSizeCount = sizeof(poolSize) / sizeof(poolSize[0]);
	poolInfo.pPoolSizes = poolSize.data();
	poolInfo.maxSets = 8192;

	if (vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor pool !");
	}

	m_frameDescriptorPools.resize(MAX_FRAMES_IN_FLIGHT, VK_NULL_HANDLE);
	VkDescriptorPoolSize framePoolSize{};
	framePoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	framePoolSize.descriptorCount = 4096;

	VkDescriptorPoolCreateInfo framePoolInfo{};
	framePoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	framePoolInfo.poolSizeCount = 1;
	framePoolInfo.pPoolSizes = &framePoolSize;
	framePoolInfo.maxSets = 4096;
	for (VkDescriptorPool& framePool : m_frameDescriptorPools)
	{
		if (vkCreateDescriptorPool(m_device, &framePoolInfo, nullptr, &framePool) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create per-frame descriptor pool !");
		}
	}
}

void icpVkGPUDevice::createFence()
{
	m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
	m_imageAvailableForRenderingSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	m_renderFinishedForPresentationSemaphores.resize(MAX_FRAMES_IN_FLIGHT);

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VkFenceCreateFlagBits::VK_FENCE_CREATE_SIGNALED_BIT;
	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		if (vkCreateFence(m_device, &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS ||
			vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_imageAvailableForRenderingSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_renderFinishedForPresentationSemaphores[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create sync objects!");
		}
	}
}

void icpVkGPUDevice::WaitForFence(uint32_t _currentFrame)
{
	if (vkWaitForFences(m_device, 1, &m_inFlightFences[_currentFrame], VK_TRUE, UINT64_MAX) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to wait for fence!");
	}
}

uint32_t icpVkGPUDevice::AcquireNextImageFromSwapchain(uint32_t _currentFrame, VkResult& _result)
{
	uint32_t imageIndex;
	_result = vkAcquireNextImageKHR(m_device, m_swapChain, UINT64_MAX, m_imageAvailableForRenderingSemaphores[_currentFrame], VK_NULL_HANDLE, &imageIndex);

	return imageIndex;
}

VkDevice& icpVkGPUDevice::GetLogicalDevice()
{
	return m_device;
}

VkPhysicalDevice& icpVkGPUDevice::GetPhysicalDevice()
{
	return m_physicalDevice;
}
VmaAllocator& icpVkGPUDevice::GetVmaAllocator()
{
	return m_vmaAllocator;
}

QueueFamilyIndices& icpVkGPUDevice::GetQueueFamilyIndices()
{
	return m_queueIndices;
}

VkCommandPool& icpVkGPUDevice::GetTransferCommandPool()
{
	return m_transferCommandPool;
}

VkQueue& icpVkGPUDevice::GetTransferQueue()
{
	return m_transferQueue;
}

VkQueue& icpVkGPUDevice::GetGraphicsQueue()
{
	return m_graphicsQueue;
}

VkQueue& icpVkGPUDevice::GetComputeQueue()
{
	return m_computeQueue;
}

VkQueue& icpVkGPUDevice::GetPresentQueue()
{
	return m_presentQueue;
}

std::vector<VkSemaphore>& icpVkGPUDevice::GetRenderFinishedForPresentationSemaphores()
{
	return m_renderFinishedForPresentationSemaphores;
}

std::vector<VkFence>& icpVkGPUDevice::GetInFlightFences()
{
	return m_inFlightFences;
}
VkDescriptorPool& icpVkGPUDevice::GetDescriptorPool()
{
	return m_descriptorPool;
}

VkInstance& icpVkGPUDevice::GetInstance()
{
	return m_instance;
}

VkSwapchainKHR& icpVkGPUDevice::GetSwapChain()
{
	return m_swapChain;
}

VkCommandPool& icpVkGPUDevice::GetGraphicsCommandPool()
{
	return m_graphicsCommandPool;
}

VkCommandPool& icpVkGPUDevice::GetComputeCommandPool()
{
	return m_computeCommandPool;
}

VkExtent2D& icpVkGPUDevice::GetSwapChainExtent()
{
	return m_swapChainExtent;
}

VkFormat icpVkGPUDevice::GetSwapChainImageFormat()
{
	return m_swapChainImageFormat;
}

std::vector<VkImageView>& icpVkGPUDevice::GetSwapChainImageViews()
{
	return m_swapChainImageViews;
}

std::vector<VkImage>& icpVkGPUDevice::GetSwapChainImages()
{
	return m_swapChainImages;
}

GLFWwindow* icpVkGPUDevice::GetWindow()
{
	return m_window;
}

VkFormat icpVkGPUDevice::GetDepthFormat()
{
	return m_depthFormat;
}

VkImageView icpVkGPUDevice::GetDepthImageView()
{
	return m_depthImageView;
}

std::vector<VkSemaphore>& icpVkGPUDevice::GetImageAvailableForRenderingSemaphores()
{
	return m_imageAvailableForRenderingSemaphores;
}

std::vector<uint32_t>& icpVkGPUDevice::GetQueueFamilyIndicesVector()
{
	return m_queueFamilyIndices;
}

uint32_t icpVkGPUDevice::GetCurrentFrameIndex() const
{
	return m_currentFrame;
}

uint32_t icpVkGPUDevice::GetBackBufferWidth() const
{
	return m_swapChainExtent.width;
}

uint32_t icpVkGPUDevice::GetBackBufferHeight() const
{
	return m_swapChainExtent.height;
}


INCEPTION_END_NAMESPACE
