#include "VulkanManager.h"

bool VulkanManager::initVulkan()
{
	std::cout << "Initializing Vulkan" << std::endl;

	createInstance();

	return false;
}

bool VulkanManager::cleanupVulkan()
{
	vkDestroyInstance(instance, nullptr);

	std::cout << "Deinitializing Vulkan" << std::endl;

	return false;
}

bool VulkanManager::createInstance()
{
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Vulkan Renderer";
	appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
	appInfo.pEngineName = "VKRen";
	appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensionNames = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	createInfo.enabledExtensionCount = glfwExtensionCount;
	createInfo.ppEnabledExtensionNames = glfwExtensionNames;
	
	createInfo.enabledLayerCount = 0;

	if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
	{
		std::cout << "ERROR: Failed to create Vulkan instance..." << std::endl;
		return false;
	}

	return true;
}
