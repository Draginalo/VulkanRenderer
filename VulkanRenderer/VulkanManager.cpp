#include "VulkanManager.h"

bool VulkanManager::initVulkan(GLFWwindow* window)
{
	std::cout << "\nInitializing Vulkan" << std::endl;

	createInstance();
	setUpDebugMessenger();
	createSurface(window);
	pickPhysicalDevice();
	createLogicalDevice();

	return false;
}

bool VulkanManager::cleanupVulkan()
{
	vkDestroyDevice(mLogicalDevice, nullptr);

	if (enableValidationLayers) 
	{
		DestroyDebugUtilsMessengerEXT(mInstance, mDebugMessanger, nullptr);
	}

	vkDestroySurfaceKHR(mInstance, mSurface, nullptr);
	vkDestroyInstance(mInstance, nullptr);

	std::cout << "\nDeinitializing Vulkan" << std::endl;

	return false;
}

bool VulkanManager::createInstance()
{
	if (enableValidationLayers && !hasValidationLayerSupport())
	{
		std::cout << "Validation layers requested but are not avalable..." << std::endl;
		return false;
	}

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

	std::vector<const char*> extensions = getRequiredExtensions();

	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();
	
	VkDebugUtilsMessengerCreateInfoEXT debugMessengerInfo{};
	if (enableValidationLayers) 
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(mValidationLayers.size());
		createInfo.ppEnabledLayerNames = mValidationLayers.data();

		populateDebugMessangerInfo(debugMessengerInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugMessengerInfo;
	}
	else 
	{
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;
	}

	if (vkCreateInstance(&createInfo, nullptr, &mInstance) != VK_SUCCESS)
	{
		std::cout << "ERROR: Failed to create Vulkan instance..." << std::endl;
		return false;
	}

	return true;
}

bool VulkanManager::hasValidationLayerSupport()
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* currLayer : mValidationLayers)
	{
		bool layerFound = false;

		for (const VkLayerProperties& availableLayer : availableLayers) 
		{
			if (strcmp(availableLayer.layerName, currLayer)) 
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

std::vector<const char*> VulkanManager::getRequiredExtensions()
{
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensionNames = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensionNames, glfwExtensionNames + glfwExtensionCount);

	if (enableValidationLayers)
	{
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;
}

bool VulkanManager::setUpDebugMessenger()
{
	if (!enableValidationLayers) return false;

	VkDebugUtilsMessengerCreateInfoEXT debugMessengerInfo{};
	populateDebugMessangerInfo(debugMessengerInfo);

	if (CreateDebugUtilsMessengerEXT(mInstance, &debugMessengerInfo, nullptr, &mDebugMessanger) != VK_SUCCESS)
	{
		std::cout << "Failed to set up debug messages." << std::endl;
		return false;
	}

	return true;
}

void VulkanManager::populateDebugMessangerInfo(VkDebugUtilsMessengerCreateInfoEXT& debugMessengerInfo)
{
	debugMessengerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	debugMessengerInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	debugMessengerInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
		| VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
		| VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	debugMessengerInfo.pfnUserCallback = debugCallback;
	debugMessengerInfo.pUserData = nullptr;
}

VkResult VulkanManager::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAlloator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	//Makes function pointer to the create debug utils messenger function
	VkResult(*func)(VkInstance, const VkDebugUtilsMessengerCreateInfoEXT*,
		const VkAllocationCallbacks*, VkDebugUtilsMessengerEXT*) =
		(PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

	if (func != nullptr) {
		return func(instance, pCreateInfo, pAlloator, pDebugMessenger);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void VulkanManager::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
	void (*func)(VkInstance, VkDebugUtilsMessengerEXT,
		const VkAllocationCallbacks*) = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");

	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}

bool VulkanManager::pickPhysicalDevice()
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(mInstance, &deviceCount, nullptr);
	if (deviceCount == 0)
	{
		return false;
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(mInstance, &deviceCount, devices.data());

	for (VkPhysicalDevice currDevice : devices)
	{
		if (deviceIsSuitable(currDevice))
		{
			mPhysicalDevice = currDevice;
			return true;
		}
	}

	std::cout << "\nFailed to find suitable physical device..." << std::endl;
	return false;
}

bool VulkanManager::deviceIsSuitable(VkPhysicalDevice device)
{
	VkPhysicalDeviceProperties deviceProperties{};

	vkGetPhysicalDeviceProperties(device, &deviceProperties);

	return deviceProperties.apiVersion >= VK_API_VERSION_1_3 
		&& findSuitableQueueFamilies(device).containsAllFamilies()
		&& supportsDeviceExtensions(device)
		&& supportsDeviceFeatures(device);
}

QueueFamiliesIndexStore VulkanManager::findSuitableQueueFamilies(VkPhysicalDevice device)
{
	QueueFamiliesIndexStore queueIndexInfo{};
	queueIndexInfo.graphicsFamalyIndex = -1;
	queueIndexInfo.presentFamalyIndex = -1;

	uint32_t queuePropertiesCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queuePropertiesCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueProperties(queuePropertiesCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queuePropertiesCount, queueProperties.data());

	int i = 0;
	//Checks for graphics support
	for (VkQueueFamilyProperties queueProperty : queueProperties)
	{
		if (queueProperty.queueFlags & VK_QUEUE_GRAPHICS_BIT) 
		{
			queueIndexInfo.graphicsFamalyIndex = i;
		}

		VkBool32 presentSupported = VK_FALSE;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, mSurface, &presentSupported);

		if (presentSupported)
		{
			queueIndexInfo.presentFamalyIndex = i;
		}

		if (queueIndexInfo.containsAllFamilies())
		{
			break;
		}

		i++;
	}

	return queueIndexInfo;
}

bool VulkanManager::supportsDeviceExtensions(VkPhysicalDevice device)
{
	uint32_t queueExtensionsCount = 0;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &queueExtensionsCount, nullptr);

	std::vector<VkExtensionProperties> queueExtensions(queueExtensionsCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &queueExtensionsCount, queueExtensions.data());

	//Checks for extension support
	for (const char* requiredExtension : mRequiredDeviceExtension)
	{
		bool supportsCurrExtension = false;
		for (const VkExtensionProperties& queueExtension : queueExtensions)
		{
			if (strcmp(requiredExtension, queueExtension.extensionName))
			{
				supportsCurrExtension = true;
				break;
			}
		}

		if (!supportsCurrExtension)
		{
			return false;
		}
	}

	return true;
}

bool VulkanManager::supportsDeviceFeatures(VkPhysicalDevice device)
{
	//Checks for feature support
	VkPhysicalDeviceFeatures2 deviceFeatures{};
	VkPhysicalDeviceVulkan13Features deviceVulkan13Features{};
	VkPhysicalDeviceExtendedDynamicStateFeaturesEXT deviceExtendedStateFeatures{};

	deviceExtendedStateFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT;

	deviceVulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
	deviceVulkan13Features.pNext = (VkPhysicalDeviceDynamicRenderingFeatures*)&deviceExtendedStateFeatures;

	deviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	deviceFeatures.pNext = (VkPhysicalDeviceVulkan13Features*)&deviceVulkan13Features;

	vkGetPhysicalDeviceFeatures2(device, &deviceFeatures);

	return deviceVulkan13Features.dynamicRendering && deviceExtendedStateFeatures.extendedDynamicState;
}

bool VulkanManager::createLogicalDevice()
{
	//Can replace this with the previously found queue index info when determining queue family suitability
	QueueFamiliesIndexStore queueFamilyIndeciesInfo = findSuitableQueueFamilies(mPhysicalDevice);
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> queueFamilyIndecies = queueFamilyIndeciesInfo.getVectorOfIndecies();

	float queuePriority = 1.0;

	for (uint32_t queueIndex : queueFamilyIndecies)
	{
		VkDeviceQueueCreateInfo deviceQueueCreateInfo{};
		deviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		deviceQueueCreateInfo.queueFamilyIndex = queueIndex;
		deviceQueueCreateInfo.queueCount = 1;
		deviceQueueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(deviceQueueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceBaseFeatures{};

	VkPhysicalDeviceFeatures2 deviceAdditFeatures{};
	VkPhysicalDeviceVulkan13Features deviceVulkan13Features{};
	VkPhysicalDeviceExtendedDynamicStateFeaturesEXT deviceExtendedStateFeatures{};

	deviceExtendedStateFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT;

	deviceVulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
	deviceVulkan13Features.pNext = (VkPhysicalDeviceDynamicRenderingFeatures*)&deviceExtendedStateFeatures;

	deviceAdditFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	deviceAdditFeatures.pNext = (VkPhysicalDeviceVulkan13Features*)&deviceVulkan13Features;

	VkDeviceCreateInfo deviceCreateInfo{};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount = 1;
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
	//deviceCreateInfo.pEnabledFeatures = &deviceBaseFeatures; //This is null because we ask for additional features in pNext
	deviceCreateInfo.pNext = (VkPhysicalDeviceFeatures2*)&deviceAdditFeatures;
	deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(mRequiredDeviceExtension.size());
	deviceCreateInfo.ppEnabledExtensionNames = mRequiredDeviceExtension.data();

	////Device Layers have never worked since Vulkan 1.0 and only Instance Layers should be used instead: 
	// https://docs.vulkan.org/spec/latest/appendices/legacy.html#legacy-devicelayers
	/*if (enableValidationLayers)
	{
		deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(mValidationLayers.size());
		deviceCreateInfo.ppEnabledLayerNames = mValidationLayers.data();
	}
	else*/
	{
		deviceCreateInfo.enabledLayerCount = 0;
	}

	if (vkCreateDevice(mPhysicalDevice, &deviceCreateInfo, nullptr, &mLogicalDevice) != VK_SUCCESS)
	{
		std::cout << "\nFailed to create logical device..." << std::endl;
		return false;
	}

	vkGetDeviceQueue(mLogicalDevice, queueFamilyIndeciesInfo.graphicsFamalyIndex, 0, &mGraphicsQueue);
	vkGetDeviceQueue(mLogicalDevice, queueFamilyIndeciesInfo.presentFamalyIndex, 0, &mPresentQueue);

	return true;
}

bool VulkanManager::createSurface(GLFWwindow* window)
{
	if (glfwCreateWindowSurface(mInstance, window, nullptr, &mSurface) != VK_SUCCESS)
	{
		std::cout << "\nFailed to create window surface..." << std::endl;
		return false;
	}

	return true;
}
