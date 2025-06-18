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
#include <limits>
#include <algorithm>
#include <cstdint>

#include "../icpDescirptorSet.h"
#include "../icpGPUBuffer.h"
#include "../../../core/icpConfigSystem.h"
#include "../../material/icpTextureRenderResourceManager.h"


INCEPTION_BEGIN_NAMESPACE
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
	createSyncObjects();

	return true;
}

void icpVkGPUDevice::createVmaAllocator()
{
	VmaAllocatorCreateInfo vma_create_info{};
	vma_create_info.vulkanApiVersion = VK_API_VERSION_1_3;
	vma_create_info.device = m_device;
	vma_create_info.instance = m_instance;
	vma_create_info.physicalDevice = m_physicalDevice;

	VkResult result = vmaCreateAllocator(&vma_create_info, &m_vmaAllocator);
}

void icpVkGPUDevice::createInstance()
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

void icpVkGPUDevice::cleanup()
{
	CleanUpSwapChain();

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

void icpVkGPUDevice::CleanUpSwapChain()
{
	vkDestroyImageView(m_device, m_depthImageView, nullptr);
	vmaDestroyImage(m_vmaAllocator, m_depthImage, m_depthBufferAllocation);

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
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

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
	std::array<VkDescriptorPoolSize, 3> poolSize{};
	poolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSize[0].descriptorCount = 500;
	poolSize[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSize[1].descriptorCount = 500;
	poolSize[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	poolSize[2].descriptorCount = 500;

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	poolInfo.poolSizeCount = sizeof(poolSize) / sizeof(poolSize[0]);
	poolInfo.pPoolSizes = poolSize.data();
	poolInfo.maxSets = 1000;

	if (vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor pool !");
	}
}

void icpVkGPUDevice::createSyncObjects()
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

void icpVkGPUDevice::WaitForFence(uint32_t _currentFrame)
{
	if (vkWaitForFences(m_device, 1, &m_inFlightFences[_currentFrame], VK_TRUE, UINT64_MAX) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to wait for fence!");
	}
	vkResetFences(m_device, 1, &m_inFlightFences[_currentFrame]);
}

uint32_t icpVkGPUDevice::AcquireNextImageFromSwapchain(uint32_t _currentFrame, VkResult& _result)
{
	uint32_t imageIndex;
	_result = vkAcquireNextImageKHR(m_device, m_swapChain, UINT64_MAX, m_imageAvailableForRenderingSemaphores[_currentFrame], VK_NULL_HANDLE, &imageIndex);

	return imageIndex;
}

void icpVkGPUDevice::CreateDescriptorSet(const icpDescriptorSetCreation& creation, std::vector<VkDescriptorSet>& DSs)
{
	std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, creation.layoutInfo.layout);

	VkDescriptorSetAllocateInfo allocateInfo{};
	allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocateInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
	allocateInfo.descriptorPool = m_descriptorPool;
	allocateInfo.pSetLayouts = layouts.data();

	DSs.resize(MAX_FRAMES_IN_FLIGHT);

	if (vkAllocateDescriptorSets(m_device, &allocateInfo, DSs.data()) != VK_SUCCESS)
	{
		ICP_LOG_FATAL("vkAllocateDescriptorSets failed!");
	}

	auto bindingSize = creation.resources.size() / MAX_FRAMES_IN_FLIGHT;

	for (int frame = 0; frame < MAX_FRAMES_IN_FLIGHT; frame++)
	{
		std::vector<VkWriteDescriptorSet> descriptorWrites(bindingSize);

		std::vector<VkDescriptorBufferInfo> bufferInfos(bindingSize);
		std::vector<VkDescriptorImageInfo> imageInfos(bindingSize);
		for (int i = 0; i < bindingSize; i ++)
		{
			auto layout = creation.layoutInfo.bindings[i];

			switch (layout.type)
			{
			case VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
				{
					auto bufferRes = std::get<icpBufferRenderResourceInfo>(creation.resources[i * 3 + frame]);
					bufferInfos[i].buffer = bufferRes.buffer;
					bufferInfos[i].offset = bufferRes.offset;
					bufferInfos[i].range = bufferRes.range;
					descriptorWrites[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					descriptorWrites[i].dstSet = DSs[frame];
					descriptorWrites[i].dstBinding = i;
					descriptorWrites[i].dstArrayElement = 0;
					descriptorWrites[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
					descriptorWrites[i].descriptorCount = 1;
					descriptorWrites[i].pBufferInfo = &bufferInfos[i];
				}
				break;
			case VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
				{
					auto imageRes = std::get<icpTextureRenderResourceInfo>(creation.resources[i * 3 + frame]);
					imageInfos[i].sampler = imageRes.m_texSampler;
					imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
					imageInfos[i].imageView = imageRes.m_texImageView;
					descriptorWrites[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					descriptorWrites[i].dstSet = DSs[frame];
					descriptorWrites[i].dstBinding = i;
					descriptorWrites[i].dstArrayElement = 0;
					descriptorWrites[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					descriptorWrites[i].descriptorCount = 1;
					descriptorWrites[i].pImageInfo = &imageInfos[i];
				}
				break;
			case VkDescriptorType::VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
				{
					auto imageRes = std::get<icpTextureRenderResourceInfo>(creation.resources[i * 3 + frame]);
					imageInfos[i].sampler = VK_NULL_HANDLE;
					imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
					imageInfos[i].imageView = imageRes.m_texImageView;
					descriptorWrites[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					descriptorWrites[i].dstSet = DSs[frame];
					descriptorWrites[i].dstBinding = i;
					descriptorWrites[i].dstArrayElement = 0;
					descriptorWrites[i].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
					descriptorWrites[i].descriptorCount = 1;
					descriptorWrites[i].pImageInfo = &imageInfos[i];
				}
				break;
			default:
				{
					ICP_LOG_WARING("not implemented yet");
				}
				break;
			}
		}

		vkUpdateDescriptorSets(GetLogicalDevice(), descriptorWrites.size(), descriptorWrites.data(),
			0, nullptr);
	}
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


INCEPTION_END_NAMESPACE