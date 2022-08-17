#pragma once
#include "icpMacros.h"
#include "icpRHI.h"
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

INCEPTION_BEGIN_NAMESPACE

class icpVulkanRHI : public icpRHIBase
{
public:
	icpVulkanRHI() = default;
	~icpVulkanRHI() override;

	bool initialize(std::shared_ptr<icpWindowSystem> window_system) override;
	void cleanup();
	

private:
	void createInstance();
	void initializeDebugMessenger();
	void createWindowSurface();

	bool checkValidationLayerSupport();

	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
	VkResult createDebugUtilsMessengerEXT(VkInstance instance,
		const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
		const VkAllocationCallbacks* pAllocator,
		VkDebugUtilsMessengerEXT* pDebugMessenger);

	void destroyDebugUtilsMessengerEXT(VkInstance instance,
		VkDebugUtilsMessengerEXT     debugMessenger,
		const VkAllocationCallbacks* pAllocator);

	VkInstance m_instance{ VK_NULL_HANDLE };
	VkSurfaceKHR m_surface{ VK_NULL_HANDLE };
	GLFWwindow* m_window{ VK_NULL_HANDLE };

	const std::vector<const char*> m_validationLayers{ "VK_LAYER_KHRONOS_validation" };
	VkDebugUtilsMessengerEXT m_debugMessenger{ VK_NULL_HANDLE };
	bool m_enableValidationLayers = true;
	bool m_enableDebugUtilsLabel = true;

	// debug utilities label
	PFN_vkCmdBeginDebugUtilsLabelEXT m_vk_cmd_begin_debug_utils_label_ext = nullptr;
	PFN_vkCmdEndDebugUtilsLabelEXT   m_vk_cmd_end_debug_utils_label_ext = nullptr;
};

INCEPTION_END_NAMESPACE