#include "icpVulkanRHI.h"
#include "../core/icpSystemContainer.h"
#include "../resource/icpResourceSystem.h"
#include "../mesh/icpMeshResource.h"
#include "icpCameraSystem.h"
#include "icpVulkanUtility.h"
#include "icpImageResource.h"
#include <iostream>
#include <map>
#include <set>
#include <limits>
#include <algorithm>
#include <cstdint>

#include "../core/icpConfigSystem.h"

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
	createWindowSurface();
	initializePhysicalDevice();
	createLogicalDevice();

	createSwapChain();
	createSwapChainImageViews();
	createDescriptorSetLayout();
	createCommandPools();
	createDepthResources();

	createTextureImages();
	createTextureSampler();
	createObjModels();
	createVertexBuffers();
	createIndexBuffers();
	createUniformBuffers();

	createDescriptorPools();
	allocateDescriptorSets();
	allocateCommandBuffers();

	createSyncObjects();

	return true;
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

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) 
	{
		vkDestroyBuffer(m_device, m_uniformBuffers[i], nullptr);
		vkFreeMemory(m_device, m_uniformBufferMem[i], nullptr);
	}

	vkDestroySampler(m_device, m_textureSampler, nullptr);
	vkDestroyImageView(m_device, m_textureImageView, nullptr);

	vkDestroyImage(m_device, m_textureImage, nullptr);
	vkFreeMemory(m_device, m_textureBufferMem, nullptr);

	vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout, nullptr);

	vkDestroyBuffer(m_device, m_vertexBuffer, nullptr);
	vkFreeMemory(m_device, m_vertexBufferMem, nullptr);

	vkDestroyBuffer(m_device, m_indexBuffer, nullptr);
	vkFreeMemory(m_device, m_indexBufferMem, nullptr);

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

void icpVulkanRHI::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
	VkCommandBuffer commandBuffer = icpVulkanUtility::beginSingleTimeCommands(m_transferCommandPool, m_device);

	VkBufferCopy copyRegin{};
	copyRegin.srcOffset = 0;
	copyRegin.dstOffset = 0;
	copyRegin.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegin);

	icpVulkanUtility::endSingleTimeCommandsAndSubmit(
		commandBuffer,
		m_transferQueue,
		m_transferCommandPool,
		m_device
	);
}

void icpVulkanRHI::copyBuffer2Image(VkBuffer srcBuffer, VkImage dstImage, uint32_t width, uint32_t height)
{
	VkCommandBuffer commandBuffer = icpVulkanUtility::beginSingleTimeCommands(m_transferCommandPool, m_device);

	VkBufferImageCopy copyRegin{};
	copyRegin.bufferOffset = 0;
	copyRegin.bufferImageHeight = 0;
	copyRegin.bufferRowLength = 0;
	copyRegin.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copyRegin.imageSubresource.mipLevel = 0;
	copyRegin.imageSubresource.layerCount = 1;
	copyRegin.imageSubresource.baseArrayLayer = 0;
	copyRegin.imageOffset = {0,0,0};
	copyRegin.imageExtent = { width, height, 1 };

	vkCmdCopyBufferToImage(commandBuffer, srcBuffer, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegin);

	icpVulkanUtility::endSingleTimeCommandsAndSubmit(
		commandBuffer,
		m_transferQueue,
		m_transferCommandPool,
		m_device
	);
}


void icpVulkanRHI::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipmapLevel)
{
	VkCommandBuffer commandBuffer = icpVulkanUtility::beginSingleTimeCommands(m_transferCommandPool, m_device);

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;

	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = mipmapLevel;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else {
		throw std::invalid_argument("unsupported layout transition!");
	}

	vkCmdPipelineBarrier(
		commandBuffer,
		sourceStage, destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	icpVulkanUtility::endSingleTimeCommandsAndSubmit(commandBuffer, m_transferQueue, m_transferCommandPool, m_device);
}

void icpVulkanRHI::createObjModels()
{
	auto objPath = g_system_container.m_configSystem->m_modelResourcePath / "viking_room.obj";
	g_system_container.m_resourceSystem->loadObjModelResource(objPath);
}


void icpVulkanRHI::createVertexBuffers()
{
	auto objPair = *(g_system_container.m_resourceSystem->m_resources.m_allResources.begin());
	auto meshP = std::dynamic_pointer_cast<icpMeshResource>(objPair.second);

	auto bufferSize = sizeof(meshP->m_meshData.m_vertices[0]) * meshP->m_meshData.m_vertices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMem;

	VkSharingMode mode = m_queueIndices.m_graphicsFamily.value() == m_queueIndices.m_transferFamily.value() ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;

	icpVulkanUtility::createVulkanBuffer(bufferSize,
		mode,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer,
		stagingBufferMem,
		m_device,
		m_physicalDevice);

	void* data;
	vkMapMemory(m_device, stagingBufferMem, 0, bufferSize, 0, &data);
	memcpy(data, meshP->m_meshData.m_vertices.data(), (size_t)bufferSize);
	vkUnmapMemory(m_device, stagingBufferMem);

	icpVulkanUtility::createVulkanBuffer(bufferSize,
		mode,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_vertexBuffer,
		m_vertexBufferMem,
		m_device,
		m_physicalDevice);

	copyBuffer(stagingBuffer, m_vertexBuffer, bufferSize);

	vkDestroyBuffer(m_device, stagingBuffer, nullptr);
	vkFreeMemory(m_device, stagingBufferMem, nullptr);
}

void icpVulkanRHI::createIndexBuffers()
{
	auto objPair = *(g_system_container.m_resourceSystem->m_resources.m_allResources.begin());
	auto meshP = std::dynamic_pointer_cast<icpMeshResource>(objPair.second);

	VkDeviceSize bufferSize = sizeof(meshP->m_meshData.m_vertexIndices[0]) * meshP->m_meshData.m_vertexIndices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMem;

	VkSharingMode mode = m_queueIndices.m_graphicsFamily.value() == m_queueIndices.m_transferFamily.value() ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;

	icpVulkanUtility::createVulkanBuffer(
		bufferSize,
		mode,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD,
		stagingBuffer,
		stagingBufferMem,
		m_device,
		m_physicalDevice);

	void* data;
	vkMapMemory(m_device, stagingBufferMem, 0, bufferSize, 0, &data);
	memcpy(data, meshP->m_meshData.m_vertexIndices.data(), (size_t)bufferSize);
	vkUnmapMemory(m_device, stagingBufferMem);

	icpVulkanUtility::createVulkanBuffer(bufferSize,
		mode,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_indexBuffer,
		m_indexBufferMem,
		m_device,
		m_physicalDevice);

	copyBuffer(stagingBuffer, m_indexBuffer, bufferSize);

	vkDestroyBuffer(m_device, stagingBuffer, nullptr);
	vkFreeMemory(m_device, stagingBufferMem, nullptr);
}

void icpVulkanRHI::createUniformBuffers()
{
	auto bufferSize = sizeof(UniformBufferObject);

	m_uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	m_uniformBufferMem.resize(MAX_FRAMES_IN_FLIGHT);

	VkSharingMode mode = m_queueIndices.m_graphicsFamily.value() == m_queueIndices.m_transferFamily.value() ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		icpVulkanUtility::createVulkanBuffer(
			bufferSize,
			mode,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD,
			m_uniformBuffers[i],
			m_uniformBufferMem[i],
			m_device,
			m_physicalDevice);
	}
}

void icpVulkanRHI::createTextureImages()
{
	auto imgPath = g_system_container.m_configSystem->m_imageResourcePath / "viking_room.png";
	g_system_container.m_resourceSystem->loadImageResource(imgPath);

	auto imgP = std::dynamic_pointer_cast<icpImageResource>(g_system_container.m_resourceSystem->m_resources.m_allResources["viking_room_img"]);

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMem;

	icpVulkanUtility::createVulkanBuffer(
		imgP->getImgBuffer().size(),
		VK_SHARING_MODE_EXCLUSIVE,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer,
		stagingBufferMem,
		m_device,
		m_physicalDevice
	);

	void* data;
	vkMapMemory(m_device, stagingBufferMem, 0, static_cast<uint32_t>(imgP->getImgBuffer().size()), 0, &data);
	memcpy(data, imgP->getImgBuffer().data(), imgP->getImgBuffer().size());
	vkUnmapMemory(m_device, stagingBufferMem);

	icpVulkanUtility::createVulkanImage(
		static_cast<uint32_t>(imgP->m_imgWidth),
		static_cast<uint32_t>(imgP->m_height),
		static_cast<uint32_t>(imgP->m_mipmapLevel),
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_textureImage,
		m_textureBufferMem,
		m_device,
		m_physicalDevice
	);


	transitionImageLayout(m_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<uint32_t>(imgP->m_mipmapLevel));
	copyBuffer2Image(stagingBuffer, m_textureImage, static_cast<uint32_t>(imgP->m_imgWidth), static_cast<uint32_t>(imgP->m_height));
	//transitionImageLayout(m_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, static_cast<uint32_t>(imgP->m_mipmapLevel));

	vkDestroyBuffer(m_device, stagingBuffer, nullptr);
	vkFreeMemory(m_device, stagingBufferMem, nullptr);

	generateMipmaps(m_textureImage, VK_FORMAT_R8G8B8A8_SRGB, static_cast<uint32_t>(imgP->m_imgWidth), static_cast<uint32_t>(imgP->m_height), static_cast<uint32_t>(imgP->m_mipmapLevel));

	createTextureImageViews(imgP->m_mipmapLevel);
}

void icpVulkanRHI::createTextureImageViews(size_t mipmaplevel)
{
	m_textureImageView = icpVulkanUtility::createImageView(m_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, mipmaplevel,m_device);
}

void icpVulkanRHI::createTextureSampler()
{
	VkSamplerCreateInfo sampler{};
	sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler.magFilter = VkFilter::VK_FILTER_LINEAR;
	sampler.minFilter = VkFilter::VK_FILTER_LINEAR;

	sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

	sampler.anisotropyEnable = VK_TRUE;

	VkPhysicalDeviceProperties properties;
	vkGetPhysicalDeviceProperties(m_physicalDevice, &properties);

	sampler.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
	sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;

	sampler.unnormalizedCoordinates = VK_FALSE;
	sampler.compareEnable = VK_FALSE;
	sampler.compareOp = VK_COMPARE_OP_ALWAYS;

	auto imgP = std::dynamic_pointer_cast<icpImageResource>(g_system_container.m_resourceSystem->m_resources.m_allResources["viking_room_img"]);
	sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler.mipLodBias = 0.0f;
	sampler.minLod = 0.0f;
	sampler.maxLod = static_cast<float>(imgP->m_mipmapLevel);

	if (vkCreateSampler(m_device, &sampler, nullptr, &m_textureSampler) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create sampler!");
	}
}

void icpVulkanRHI::generateMipmaps(VkImage image, VkFormat imageFormat, int32_t width, int32_t height, uint32_t mipmapLevels)
{
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(m_physicalDevice, imageFormat,
		&formatProperties);

	if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) 
	{
		throw std::runtime_error("texture image format does not support linear blitting!");
	}

	VkCommandBuffer cb = icpVulkanUtility::beginSingleTimeCommands(m_graphicsCommandPool, m_device);

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = image;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.layerCount = 1;

	int32_t mipmapWidth = width;
	int32_t mipmapHeight = height;

	for (uint32_t i = 1; i < mipmapLevels; i++)
	{
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		vkCmdPipelineBarrier(cb,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		VkImageBlit blit{};
		blit.srcOffsets[0] = { 0, 0, 0 };
		blit.srcOffsets[1] = { mipmapWidth, mipmapHeight, 1 };
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = i - 1;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;
		blit.dstOffsets[0] = { 0, 0, 0 };
		blit.dstOffsets[1] = { mipmapWidth > 1 ? mipmapWidth / 2 : 1, mipmapHeight > 1 ? mipmapHeight / 2 : 1, 1 };
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = i;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;

		vkCmdBlitImage(cb,
			image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &blit,
			VK_FILTER_LINEAR);

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(cb,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		if (mipmapWidth > 1) mipmapWidth /= 2;
		if (mipmapHeight > 1) mipmapHeight /= 2;
	}

	barrier.subresourceRange.baseMipLevel = mipmapLevels - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(cb,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
		0, nullptr,
		0, nullptr,
		1, &barrier);

	icpVulkanUtility::endSingleTimeCommandsAndSubmit(cb, m_graphicsQueue, m_graphicsCommandPool, m_device);
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
	std::array<VkDescriptorPoolSize, 2> poolSize{};
	poolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSize[0].descriptorCount = 100;
	poolSize[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSize[1].descriptorCount = 100;

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

void icpVulkanRHI::allocateDescriptorSets()
{
	std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, m_descriptorSetLayout);

	VkDescriptorSetAllocateInfo allocateInfo{};
	allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocateInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
	allocateInfo.descriptorPool = m_descriptorPool;
	allocateInfo.pSetLayouts = layouts.data();

	m_descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

	if (vkAllocateDescriptorSets(m_device, &allocateInfo, m_descriptorSets.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate descriptor sets!");
	}

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) 
	{
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = m_uniformBuffers[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);

		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageView = m_textureImageView;
		imageInfo.sampler = m_textureSampler;
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = m_descriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = m_descriptorSets[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pImageInfo = &imageInfo;

		vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
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

	m_viewportCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	VkCommandBufferAllocateInfo viewportAllocInfo{};
	viewportAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	viewportAllocInfo.commandPool = m_transferCommandPool;
	viewportAllocInfo.commandBufferCount = (uint32_t)m_viewportCommandBuffers.size();
	viewportAllocInfo.level = VkCommandBufferLevel::VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	if (vkAllocateCommandBuffers(m_device, &viewportAllocInfo, m_viewportCommandBuffers.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate viewport command buffer!");
	}

}

void icpVulkanRHI::createDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding uboLayoutBinding{};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.pImmutableSamplers = nullptr;
	uboLayoutBinding.stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutBinding samplerLayoutBinding{};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.pImmutableSamplers = nullptr;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 2> bindings{ uboLayoutBinding, samplerLayoutBinding };

	VkDescriptorSetLayoutCreateInfo createInfo{};
	createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	createInfo.pBindings = bindings.data();
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

	if (vkCreateDescriptorSetLayout(m_device, &createInfo, nullptr, &m_descriptorSetLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor set layout!");
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

void icpVulkanRHI::updateUniformBuffers(uint32_t _curImage)
{
	static auto startTime = std::chrono::high_resolution_clock::now();
	auto current = std::chrono::high_resolution_clock::now();

	float time = std::chrono::duration<float, std::chrono::seconds::period>(current - startTime).count();

	auto camera = g_system_container.m_cameraSystem->getCurrentCamera();

	UniformBufferObject ubo{};
	auto firstRotate = glm::rotate(glm::mat4(1.f), glm::radians(-90.0f), glm::vec3(0.f, 0.f, 1.f));
	auto secondRotate = glm::rotate(glm::mat4(1.f), glm::radians(-90.0f), glm::vec3(1.f, 0.f, 0.f));
	//ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	ubo.model = secondRotate * firstRotate ; //glm::mat4(1.0f);
	ubo.view = g_system_container.m_cameraSystem->getCameraViewMatrix(camera);
	auto aspectRatio = m_swapChainExtent.width / (float)m_swapChainExtent.height;
	ubo.projection = glm::perspective(camera->m_fov, aspectRatio, camera->m_near, camera->m_far);
	ubo.projection[1][1] *= -1;

	void* data;
	vkMapMemory(m_device, m_uniformBufferMem[_curImage], 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(m_device, m_uniformBufferMem[_curImage]);
}



INCEPTION_END_NAMESPACE