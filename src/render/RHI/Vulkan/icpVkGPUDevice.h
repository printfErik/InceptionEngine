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
	void cleanupSwapChain();

	void waitForFence(uint32_t _currentFrame);
	uint32_t acquireNextImageFromSwapchain(uint32_t _currentFrame, VkResult& _result);
	void resetCommandBuffer(uint32_t _currentFrame);

	void createSwapChain();
	void createSwapChainImageViews();

	void createDepthResources();
	void createDescriptorPools();

	void allocateCommandBuffers();

	void createVmaAllocator();

	VkDevice GetLogicalDevice() override;

	void CreateDescriptorSet(const icpDescriptorSetCreation& creation, std::vector<VkDescriptorSet>& DSs) override;

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
	GLFWwindow* m_window{ VK_NULL_HANDLE };
	VkPhysicalDevice m_physicalDevice{ VK_NULL_HANDLE };
	QueueFamilyIndices m_queueIndices;
	VkDevice m_device{ VK_NULL_HANDLE };
	VkQueue  m_graphicsQueue{ VK_NULL_HANDLE };
	VkQueue m_presentQueue{ VK_NULL_HANDLE };
	VkQueue m_transferQueue{ VK_NULL_HANDLE };

	VkSwapchainKHR m_swapChain{ VK_NULL_HANDLE };
	std::vector<VkImage> m_swapChainImages;
	std::vector<VkImageView> m_swapChainImageViews;
	VkFormat m_swapChainImageFormat{ VK_FORMAT_UNDEFINED };
	VkExtent2D m_swapChainExtent;

	VkCommandPool m_graphicsCommandPool{ VK_NULL_HANDLE };
	VkCommandPool m_transferCommandPool{ VK_NULL_HANDLE };
	VkCommandPool m_uiCommandPool{ VK_NULL_HANDLE };

	std::vector<VkCommandBuffer> m_graphicsCommandBuffers;
	std::vector<VkCommandBuffer> m_transferCommandBuffers;
	std::vector<VkCommandBuffer> m_uiCommandBuffers;
	/*std::vector<VkCommandBuffer> m_viewportCommandBuffers;*/

	VkImage m_depthImage;
	VmaAllocation m_depthBufferAllocation;

	VkImageView m_depthImageView;

	std::vector<VkSemaphore> m_imageAvailableForRenderingSemaphores;
	std::vector<VkSemaphore> m_renderFinishedForPresentationSemaphores;
	std::vector<VkFence> m_inFlightFences;

	VkDescriptorPool m_descriptorPool{ VK_NULL_HANDLE };

	VmaAllocator m_vmaAllocator{ VK_NULL_HANDLE };

	bool m_framebufferResized = false;

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