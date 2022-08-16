#include "icpVulkanRHI.h"
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

INCEPTION_BEGIN_NAMESPACE

icpVulkanRHI::~icpVulkanRHI()
{
}

bool icpVulkanRHI::initialize()
{


	return true;
}

void icpVulkanRHI::createInstance()
{
	// app info
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.apiVersion = VK_API_VERSION_1_3;
	appInfo.engineVersion = VK_MAKE_API_VERSION(0, 0, 1, 0);
	appInfo.applicationVersion = VK_MAKE_API_VERSION(0, 0, 1, 0);
	appInfo.pApplicationName = "Inception_Renderer";
	appInfo.pEngineName = "Inception_Engine";

	// instance create info
	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

	createInfo.enabledExtensionCount = static_cast<uint32_t>(glfwExtensionCount);
	createInfo.ppEnabledExtensionNames = extensions.data();


}



INCEPTION_END_NAMESPACE