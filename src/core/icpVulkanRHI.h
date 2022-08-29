#pragma once
#include <optional>
#include <vector>

#include "icpMacros.h"
#include "icpRHI.h"
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

INCEPTION_BEGIN_NAMESPACE

static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 3;

struct QueueFamilyIndices
{
	std::optional<uint32_t> m_graphicsFamily;
	std::optional<uint32_t> m_presentFamily;

	bool isComplete() const { return m_graphicsFamily.has_value() && m_presentFamily.has_value(); }
};

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR        m_capabilities{};
	std::vector<VkSurfaceFormatKHR> m_formats;
	std::vector<VkPresentModeKHR>   m_presentModes;
};

class icpVulkanRHI : public icpRHIBase
{
public:
	icpVulkanRHI() = default;
	~icpVulkanRHI() override;

	bool initialize(std::shared_ptr<icpWindowSystem> window_system) override;
	void cleanup();

	void waitForFence(uint32_t _currentFrame);
	uint32_t acquireNextImageFromSwapchain(uint32_t _currentFrame);
	void resetCommandBuffer(uint32_t _currentFrame);
	void submitRendering(uint32_t _imageIndex, uint32_t _currentFrame);

private:
	void createInstance();
	void initializeDebugMessenger();
	void createWindowSurface();
	void initializePhysicalDevice();
	void createLogicalDevice();
	void createSwapChain();
	void createSwapChainImageViews();

	void createCommandPool();
	void allocateCommandBuffers();

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


	VkSwapchainKHR m_swapChain{ VK_NULL_HANDLE };
	std::vector<VkImage> m_swapChainImages;
	std::vector<VkImageView> m_swapChainImageViews;
	VkFormat m_swapChainImageFormat{ VK_FORMAT_UNDEFINED };
	VkExtent2D m_swapChainExtent;

	VkCommandPool m_commandPool{ VK_NULL_HANDLE };
	std::vector<VkCommandBuffer> m_commandBuffers;

	std::vector<VkSemaphore> m_imageAvailableForRenderingSemaphores;
	std::vector<VkSemaphore> m_renderFinishedForPresentationSemaphores;
	std::vector<VkFence> m_inFlightFences;

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