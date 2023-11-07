#pragma once
#include <optional>
#include <vector>
#include <chrono>

#include "../icpGPUDevice.h"
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

#include "vk_mem_alloc.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

INCEPTION_BEGIN_NAMESPACE

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR        m_capabilities{};
	std::vector<VkSurfaceFormatKHR> m_formats;
	std::vector<VkPresentModeKHR>   m_presentModes;
};

class icpVkGPUDevice : public icpGPUDevice
{
public:
	icpVkGPUDevice() = default;
	~icpVkGPUDevice() override;

	bool Initialize(std::shared_ptr<icpWindowSystem> window_system) override;

	void cleanup();
	void CleanUpSwapChain() override;

	void WaitForFence(uint32_t _currentFrame) override;
	uint32_t AcquireNextImageFromSwapchain(uint32_t _currentFrame, VkResult& _result) override;

	void CreateSwapChain() override;
	void CreateSwapChainImageViews() override;

	void CreateDepthResources() override;
	void createDescriptorPools();

	void createVmaAllocator();

	VkDevice& GetLogicalDevice() override;
	VkPhysicalDevice& GetPhysicalDevice() override;
	void CreateDescriptorSet(const icpDescriptorSetCreation& creation, std::vector<VkDescriptorSet>& DSs) override;

	VmaAllocator& GetVmaAllocator() override;
	QueueFamilyIndices& GetQueueFamilyIndices() override;

	VkQueue& GetTransferQueue() override;
	VkCommandPool& GetTransferCommandPool() override;

	VkCommandPool& GetGraphicsCommandPool() override;
	VkQueue& GetGraphicsQueue() override;

	VkQueue& GetPresentQueue() override;

	VkSwapchainKHR& GetSwapChain() override;
	VkExtent2D& GetSwapChainExtent() override;
	std::vector<VkImageView>& GetSwapChainImageViews() override;
	std::vector<VkImage>& GetSwapChainImages() override;
	VkFormat GetSwapChainImageFormat() override;

	std::vector<VkSemaphore>& GetRenderFinishedForPresentationSemaphores() override;
	std::vector<VkSemaphore>& GetImageAvailableForRenderingSemaphores() override;
	std::vector<VkFence>& GetInFlightFences() override;

	VkDescriptorPool& GetDescriptorPool() override;
	VkInstance& GetInstance() override;
	
	GLFWwindow* GetWindow() override;

	VkImageView GetDepthImageView() override;

	std::vector<uint32_t>& GetQueueFamilyIndicesVector() override;
	
private:
	void createInstance();
	void initializeDebugMessenger();
	void createWindowSurface();
	void initializePhysicalDevice();
	void createLogicalDevice();
	
	void createCommandPools();

	void createSyncObjects();

	bool checkValidationLayerSupport();

	bool isDeviceSuitable(VkPhysicalDevice device);

	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

	bool checkDeviceExtensionSupport(VkPhysicalDevice device);

	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

	VkResult createDebugUtilsMessengerEXT(VkInstance instance,
		const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
		const VkAllocationCallbacks* pAllocator,
		VkDebugUtilsMessengerEXT* pDebugMessenger);

	void destroyDebugUtilsMessengerEXT(VkInstance instance,
		VkDebugUtilsMessengerEXT     debugMessenger,
		const VkAllocationCallbacks* pAllocator);

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

public:
	VkInstance m_instance{ VK_NULL_HANDLE };
	VkSurfaceKHR m_surface{ VK_NULL_HANDLE };

	VkPhysicalDevice m_physicalDevice{ VK_NULL_HANDLE };
	QueueFamilyIndices m_queueIndices;

	std::vector<uint32_t> m_queueFamilyIndices;

	VkDevice m_device{ VK_NULL_HANDLE };
	VkQueue  m_graphicsQueue{ VK_NULL_HANDLE };
	VkQueue m_presentQueue{ VK_NULL_HANDLE };
	VkQueue m_transferQueue{ VK_NULL_HANDLE };
	VkQueue m_computeQueue{ VK_NULL_HANDLE };

	VkSwapchainKHR m_swapChain{ VK_NULL_HANDLE };
	std::vector<VkImage> m_swapChainImages;
	std::vector<VkImageView> m_swapChainImageViews;
	VkFormat m_swapChainImageFormat{ VK_FORMAT_UNDEFINED };
	VkExtent2D m_swapChainExtent;

	VkCommandPool m_graphicsCommandPool{ VK_NULL_HANDLE };
	VkCommandPool m_transferCommandPool{ VK_NULL_HANDLE };
	VkCommandPool m_computeCommandPool{ VK_NULL_HANDLE };

	VkImage m_depthImage;
	VmaAllocation m_depthBufferAllocation;

	VkImageView m_depthImageView;

	std::vector<VkSemaphore> m_imageAvailableForRenderingSemaphores;
	std::vector<VkSemaphore> m_renderFinishedForPresentationSemaphores;
	std::vector<VkFence> m_inFlightFences;

	VkDescriptorPool m_descriptorPool{ VK_NULL_HANDLE };

	VmaAllocator m_vmaAllocator{ VK_NULL_HANDLE };

private:
	VkDebugUtilsMessengerEXT m_debugMessenger{ VK_NULL_HANDLE };
	bool m_enableValidationLayers = true;
	bool m_enableDebugUtilsLabel = true;

	// debug utilities label
	PFN_vkCmdBeginDebugUtilsLabelEXT m_vk_cmd_begin_debug_utils_label_ext = nullptr;
	PFN_vkCmdEndDebugUtilsLabelEXT   m_vk_cmd_end_debug_utils_label_ext = nullptr;

	const std::vector<const char*> m_validationLayers{ "VK_LAYER_KHRONOS_validation" };
	std::vector<char const*> m_requiredDeviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#if defined(__MACH__)
			"VK_KHR_portability_subset"
#endif
	};
};

INCEPTION_END_NAMESPACE