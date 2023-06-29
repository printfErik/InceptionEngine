#include "icpVulkanRHI.h"
#include "../../../core/icpSystemContainer.h"
#include "../../../resource/icpResourceSystem.h"
#include "../../../mesh/icpMeshResource.h"
#include "../../icpCameraSystem.h"
#include "icpVulkanUtility.h"
#include "../../icpImageResource.h"
#include <iostream>
#include <map>
#include <set>
#include <limits>
#include <algorithm>
#include <cstdint>

#include "../../../core/icpConfigSystem.h"


INCEPTION_BEGIN_NAMESPACE

icpVulkanRHI::~icpVulkanRHI()
{
	cleanup();
}

bool icpVulkanRHI::initialize(std::shared_ptr<icpWindowSystem> window_system)
{
	m_window = window_system->getWindow();

	createInstance();
	initializeDebugMessenger();

	createVmaAllocator();

	createWindowSurface();
	initializePhysicalDevice();
	createLogicalDevice();

	createSwapChain();
	createSwapChainImageViews();

	createCommandPools();
	createDepthResources();

	createDescriptorPools();
	
	allocateCommandBuffers();

	createSyncObjects();

	return true;
}

void icpVulkanRHI::createVmaAllocator()
{
	VmaAllocatorCreateInfo vma_create_info{};
	vma_create_info.vulkanApiVersion = VK_API_VERSION_1_3;
	vma_create_info.device = m_device;
	vma_create_info.instance = m_instance;
	vma_create_info.physicalDevice = m_physicalDevice;

	VkResult result = vmaCreateAllocator(&vma_create_info, &m_vmaAllocator);
}


void icpVulkanRHI::createInstance()
{
	if (!checkValidationLayerSupport())
	{
		std::cerr << "no validation layer support" << std::endl;
		return;
	}

	// app info
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;

#ifdef VK_VERSION_1_3
	appInfo.apiVersion = VK_API_VERSION_1_3;
#elif VK_VERSION_1_2
	appInfo.apiVersion = VK_API_VERSION_1_2;
#elif VK_VERSION_1_1
	appInfo.apiVersion = VK_API_VERSION_1_1;
#endif
	
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

void icpVulkanRHI::cleanup()
{
	cleanupSwapChain();

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkDestroySemaphore(m_device, m_imageAvailableForRenderingSemaphores[i], nullptr);
		vkDestroySemaphore(m_device, m_renderFinishedForPresentationSemaphores[i], nullptr);
		vkDestroyFence(m_device, m_inFlightFences[i], nullptr);
	}

	vkDestroyCommandPool(m_device, m_graphicsCommandPool, nullptr);
	vkDestroyCommandPool(m_device, m_transferCommandPool, nullptr);

	vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);

	vkDestroyDevice(m_device, nullptr);

	if (m_enableValidationLayers)
	{
		destroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
	}

	vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
	
	vkDestroyInstance(m_instance, nullptr);
}

void icpVulkanRHI::cleanupSwapChain()
{
	vkDestroyImageView(m_device, m_depthImageView, nullptr);
	vkDestroyImage(m_device, m_depthImage, nullptr);
	vkFreeMemory(m_device, m_depthBufferMem, nullptr);

	for (const auto& imgView : m_swapChainImageViews)
	{
		vkDestroyImageView(m_device, imgView, nullptr);
	}

	vkDestroySwapchainKHR(m_device, m_swapChain, nullptr);
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT,
	VkDebugUtilsMessageTypeFlagsEXT,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
	return VK_FALSE;
}

void icpVulkanRHI::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
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

bool icpVulkanRHI::checkValidationLayerSupport()
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

void icpVulkanRHI::initializeDebugMessenger()
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

VkResult icpVulkanRHI::createDebugUtilsMessengerEXT(VkInstance instance,
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

void icpVulkanRHI::destroyDebugUtilsMessengerEXT(VkInstance instance,
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

void icpVulkanRHI::createWindowSurface()
{
	if (glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface) != VK_SUCCESS)
	{
		throw std::runtime_error("glfwCreateWindowSurface failed");
	}
}

void icpVulkanRHI::initializePhysicalDevice()
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

bool icpVulkanRHI::isDeviceSuitable(VkPhysicalDevice device)
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

bool icpVulkanRHI::checkDeviceExtensionSupport(VkPhysicalDevice device)
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


QueueFamilyIndices icpVulkanRHI::findQueueFamilies(VkPhysicalDevice device)
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

		if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT)
		{
			indices.m_transferFamily = i;
		}

		VkBool32 isPresentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &isPresentSupport);
		if (isPresentSupport)
		{
			indices.m_presentFamily = i;
		}

		if (indices.isComplete())
		{
			break;
		}
		i++;
	}
	return indices;
}

void icpVulkanRHI::createLogicalDevice()
{
	m_queueIndices = findQueueFamilies(m_physicalDevice);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> queueFamilies = { m_queueIndices.m_graphicsFamily.value(), m_queueIndices.m_presentFamily.value(), m_queueIndices.m_transferFamily.value()};

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
}

SwapChainSupportDetails icpVulkanRHI::querySwapChainSupport(VkPhysicalDevice device)
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

VkSurfaceFormatKHR icpVulkanRHI::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
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

VkPresentModeKHR icpVulkanRHI::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
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

VkExtent2D icpVulkanRHI::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
	if (capabilities.currentExtent.width != UINT32_MAX)
	{
		return capabilities.currentExtent;
	}
	else
	{
		int width, height;
		glfwGetFramebufferSize(m_window, &width, &height);

		VkExtent2D actualExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

		actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
		return actualExtent;
	}
}

void icpVulkanRHI::createSwapChain()
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
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	uint32_t queueFamilyIndices[] = { m_queueIndices.m_graphicsFamily.value(),
		m_queueIndices.m_presentFamily.value(),
		m_queueIndices.m_transferFamily.value()};

	std::set<uint32_t> queueFamilyIndexSet = { m_queueIndices.m_graphicsFamily.value(),
		m_queueIndices.m_presentFamily.value(),
		m_queueIndices.m_transferFamily.value() };
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
}

void icpVulkanRHI::createSwapChainImageViews()
{
	m_swapChainImageViews.resize(m_swapChainImages.size());

	for (size_t i = 0; i < m_swapChainImages.size(); i++) {
		m_swapChainImageViews[i] = icpVulkanUtility::createImageView(m_swapChainImages[i], m_swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1, m_device);
	}
}

void icpVulkanRHI::createCommandPools()
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

	VkCommandPoolCreateInfo uiPoolCreateInfo = {};
	uiPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	uiPoolCreateInfo.queueFamilyIndex = m_queueIndices.m_graphicsFamily.value();
	uiPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	if (vkCreateCommandPool(m_device, &uiPoolCreateInfo, nullptr, &m_uiCommandPool) != VK_SUCCESS) {
		throw std::runtime_error("Could not create graphics command pool");
	}
}

void icpVulkanRHI::createDepthResources() {
	VkFormat depthFormat = icpVulkanUtility::findDepthFormat(m_physicalDevice);

	icpVulkanUtility::createVulkanImage(
		m_swapChainExtent.width, 
		m_swapChainExtent.height,
		1,
		depthFormat, 
		VK_IMAGE_TILING_OPTIMAL, 
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
		m_depthImage, 
		m_depthBufferMem,
		m_device,
		m_physicalDevice
	);
	m_depthImageView = icpVulkanUtility::createImageView(m_depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1, m_device);
}

bool hasStencilComponent(VkFormat format) {
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void icpVulkanRHI::createDescriptorPools()
{
	std::array<VkDescriptorPoolSize, 3> poolSize{};
	poolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSize[0].descriptorCount = 100;
	poolSize[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSize[1].descriptorCount = 100;
	poolSize[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	poolSize[2].descriptorCount = 100;

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	poolInfo.poolSizeCount = sizeof(poolSize) / sizeof(poolSize[0]);
	poolInfo.pPoolSizes = poolSize.data();
	poolInfo.maxSets = 200;

	if (vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor pool !");
	}
}

void icpVulkanRHI::allocateCommandBuffers()
{
	m_graphicsCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

	VkCommandBufferAllocateInfo gAllocInfo{};
	gAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	gAllocInfo.commandPool = m_graphicsCommandPool;
	gAllocInfo.commandBufferCount = (uint32_t)m_graphicsCommandBuffers.size();
	gAllocInfo.level = VkCommandBufferLevel::VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	if (vkAllocateCommandBuffers(m_device, &gAllocInfo, m_graphicsCommandBuffers.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate graphics command buffer!");
	}

	m_transferCommandBuffers.resize(1); // for transfer use

	VkCommandBufferAllocateInfo tAllocInfo{};
	tAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	tAllocInfo.commandPool = m_transferCommandPool;
	tAllocInfo.commandBufferCount = (uint32_t)m_transferCommandBuffers.size();
	tAllocInfo.level = VkCommandBufferLevel::VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	if (vkAllocateCommandBuffers(m_device, &tAllocInfo, m_transferCommandBuffers.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate transfer command buffer!");
	}

	m_uiCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

	VkCommandBufferAllocateInfo uiAllocInfo{};
	uiAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	uiAllocInfo.commandPool = m_uiCommandPool;
	uiAllocInfo.commandBufferCount = (uint32_t)m_uiCommandBuffers.size();
	uiAllocInfo.level = VkCommandBufferLevel::VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	if (vkAllocateCommandBuffers(m_device, &uiAllocInfo, m_uiCommandBuffers.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate ui command buffer!");
	}
}

void icpVulkanRHI::createSyncObjects()
{
	m_imageAvailableForRenderingSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	m_renderFinishedForPresentationSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VkFenceCreateFlagBits::VK_FENCE_CREATE_SIGNALED_BIT;


	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		if (vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_imageAvailableForRenderingSemaphores[i]) ||
			vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_renderFinishedForPresentationSemaphores[i]) ||
			vkCreateFence(m_device, &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create sync objects!");
		}
	}
}

void icpVulkanRHI::waitForFence(uint32_t _currentFrame)
{
	if (vkWaitForFences(m_device, 1, &m_inFlightFences[_currentFrame], VK_TRUE, UINT64_MAX) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to wait for fence!");
	}
	vkResetFences(m_device, 1, &m_inFlightFences[_currentFrame]);
}

uint32_t icpVulkanRHI::acquireNextImageFromSwapchain(uint32_t _currentFrame, VkResult& _result)
{
	uint32_t imageIndex;
	_result = vkAcquireNextImageKHR(m_device, m_swapChain, UINT64_MAX, m_imageAvailableForRenderingSemaphores[_currentFrame], VK_NULL_HANDLE, &imageIndex);

	return imageIndex;
}

void icpVulkanRHI::resetCommandBuffer(uint32_t _currentFrame)
{
	vkResetCommandBuffer(m_graphicsCommandBuffers[_currentFrame], 0);
}

INCEPTION_END_NAMESPACE