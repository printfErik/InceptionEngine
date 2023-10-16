#pragma once
#include "../../core/icpMacros.h"
#include "../icpWindowSystem.h"
#include <vk_mem_alloc.h>

INCEPTION_BEGIN_NAMESPACE

struct icpDescriptorSetCreation;

static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 3;

struct QueueFamilyIndices
{
	std::optional<uint32_t> m_graphicsFamily;
	std::optional<uint32_t> m_presentFamily;
	std::optional<uint32_t> m_transferFamily;

	bool isComplete() const
	{
		return m_graphicsFamily.has_value()
			&& m_presentFamily.has_value()
			&& m_transferFamily.has_value();
	}
};

class icpGPUDevice
{
public:
	icpGPUDevice() = default;

	virtual	~icpGPUDevice() = 0;

	virtual bool Initialize(std::shared_ptr<icpWindowSystem> window_system) = 0;

	virtual VkDevice& GetLogicalDevice() = 0;
	virtual VkPhysicalDevice& GetPhysicalDevice() = 0;
	virtual VkFormat GetSwapChainImageFormat() = 0;
	virtual VkExtent2D& GetSwapChainExtent() = 0;
	virtual std::vector<VkImageView>& GetSwapChainImageViews() = 0;
	virtual std::vector<VkImage>& GetSwapChainImages() = 0;
	virtual VkSwapchainKHR& GetSwapChain() = 0;
	virtual VkImageView GetDepthImageView() = 0;
	virtual QueueFamilyIndices& GetQueueFamilyIndices() = 0;
	virtual std::vector<VkSemaphore>& GetImageAvailableForRenderingSemaphores() = 0;
	virtual std::vector<VkSemaphore>& GetRenderFinishedForPresentationSemaphores() = 0;
	virtual std::vector<VkFence>& GetInFlightFences() = 0;
	virtual std::vector<VkCommandBuffer>& GetGraphicsCommandBuffers() = 0;
	virtual std::vector<VkCommandBuffer>& GetComputeCommandBuffers() = 0;
	virtual std::vector<VkCommandBuffer>& GetTransferCommandBuffers() = 0;
	virtual GLFWwindow* GetWindow() = 0;
	virtual VmaAllocator& GetVmaAllocator() = 0;
	virtual VkQueue& GetGraphicsQueue() = 0;
	virtual VkQueue& GetPresentQueue() = 0;
	virtual VkQueue& GetTransferQueue() = 0;
	virtual VkCommandPool& GetGraphicsCommandPool() = 0;
	virtual VkCommandPool& GetTransferCommandPool() = 0;
	virtual VkDescriptorPool& GetDescriptorPool() = 0;
	virtual VkInstance& GetInstance() = 0;

	virtual void WaitForFence(uint32_t _currentFrame) = 0;
	virtual uint32_t AcquireNextImageFromSwapchain(uint32_t _currentFrame, VkResult& _result) = 0;

	virtual void CleanUpSwapChain() = 0;
	virtual void CreateSwapChain() = 0;
	virtual void CreateSwapChainImageViews() = 0;
	virtual void CreateDepthResources() = 0;

	virtual void CreateDescriptorSet(const icpDescriptorSetCreation& creation, std::vector<VkDescriptorSet>& DSs) = 0;

	virtual void ResetCommandBuffer(uint32_t _currentFrame) = 0;

	bool m_framebufferResized = false;
	GLFWwindow* m_window{ VK_NULL_HANDLE };
};

inline icpGPUDevice::~icpGPUDevice() = default;

INCEPTION_END_NAMESPACE