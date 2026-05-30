#include "VulkanManager.h"

bool VulkanManager::initVulkan(GLFWwindow* window)
{
	std::cout << "\nInitializing Vulkan" << std::endl;

	createInstance();
	setUpDebugMessenger();
	createSurface(window);
	pickPhysicalDevice();
	createLogicalDevice();
	createSwapChain(window);
	createImageViews();
	createRenderPass();
	createGraphicsPipeline();

	return false;
}

bool VulkanManager::cleanupVulkan()
{
	vkDestroyPipeline(mLogicalDevice, mGraphicsPipeline, nullptr);
	vkDestroyPipelineLayout(mLogicalDevice, mPipelineLayout, nullptr);
	vkDestroyRenderPass(mLogicalDevice, mRenderPass, nullptr);

	for (VkImageView imageView : mSwapChainImageViews)
	{
		vkDestroyImageView(mLogicalDevice, imageView, nullptr);
	}

	vkDestroySwapchainKHR(mLogicalDevice, mSwapChain, nullptr);
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

	bool supportsExtensions = supportsDeviceExtensions(device);

	bool swapChainAdequite = false;
	if (supportsExtensions)
	{
		SwapChainSupportDetails swapChainDetails = querySwapChainSupport(device);
		swapChainAdequite = !swapChainDetails.formats.empty() && !swapChainDetails.presentModes.empty();
	}

	return deviceProperties.apiVersion >= VK_API_VERSION_1_3 
		&& findSuitableQueueFamilies(device).containsAllFamilies()
		&& supportsExtensions 
		&& swapChainAdequite
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

	std::vector<VkExtensionProperties> avalableQueueExtensions(queueExtensionsCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &queueExtensionsCount, avalableQueueExtensions.data());

	std::set<std::string> requiredExtensionNames(mRequiredDeviceExtension.begin(), mRequiredDeviceExtension.end());

	//Checks for extension support by removing extension from required extensions list when one is found
	for (const VkExtensionProperties& queueExtension : avalableQueueExtensions)
	{
		requiredExtensionNames.erase(queueExtension.extensionName);
	}

	return requiredExtensionNames.empty();
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
	deviceBaseFeatures.logicOp = true;

	VkPhysicalDeviceFeatures2 deviceAdditFeatures{};
	VkPhysicalDeviceVulkan13Features deviceVulkan13Features{};
	VkPhysicalDeviceExtendedDynamicStateFeaturesEXT deviceExtendedStateFeatures{};

	deviceExtendedStateFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT;

	deviceVulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
	deviceVulkan13Features.pNext = (VkPhysicalDeviceDynamicRenderingFeatures*)&deviceExtendedStateFeatures;

	deviceAdditFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	deviceAdditFeatures.pNext = (VkPhysicalDeviceVulkan13Features*)&deviceVulkan13Features;
	deviceAdditFeatures.features = deviceBaseFeatures;

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

SwapChainSupportDetails VulkanManager::querySwapChainSupport(VkPhysicalDevice device)
{
	SwapChainSupportDetails swapChainDetails{};

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, mSurface, &swapChainDetails.capabilities);

	uint32_t formatsCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, mSurface, &formatsCount, nullptr);

	if (formatsCount != 0)
	{
		swapChainDetails.formats.resize(formatsCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, mSurface, &formatsCount, swapChainDetails.formats.data());
	}

	uint32_t presentModesCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, mSurface, &presentModesCount, nullptr);

	if (presentModesCount != 0)
	{
		swapChainDetails.presentModes.resize(presentModesCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, mSurface, &presentModesCount, swapChainDetails.presentModes.data());
	}

	return swapChainDetails;
}

VkSurfaceFormatKHR VulkanManager::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> availableFormats)
{
	for (const VkSurfaceFormatKHR& format : availableFormats)
	{
		if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)
		{
			return format;
		}
	}

	return availableFormats[0];
}

VkPresentModeKHR VulkanManager::chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes)
{
	for (const VkPresentModeKHR& presentMode : availablePresentModes)
	{
		if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return presentMode;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanManager::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window)
{
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
	{
		return capabilities.currentExtent;
	}

	int width, height;
	glfwGetFramebufferSize(window, &width, &height);

	VkExtent2D extents = {
		static_cast<uint32_t>(width),
		static_cast<uint32_t>(height)
	};

	extents.width = std::max(std::min(extents.width, capabilities.maxImageExtent.width), capabilities.minImageExtent.width);
	extents.height = std::max(std::min(extents.height, capabilities.maxImageExtent.height), capabilities.minImageExtent.height);

	return extents;
}

bool VulkanManager::createSwapChain(GLFWwindow* window)
{
	SwapChainSupportDetails swapChainSupportDetails = querySwapChainSupport(mPhysicalDevice);

	VkSurfaceFormatKHR swapFormat = chooseSwapSurfaceFormat(swapChainSupportDetails.formats);
	VkPresentModeKHR swapPresentMode = chooseSwapPresentMode(swapChainSupportDetails.presentModes);
	VkExtent2D swapExtents = chooseSwapExtent(swapChainSupportDetails.capabilities, window);

	uint32_t imageCount = swapChainSupportDetails.capabilities.minImageCount + 1;
	if (swapChainSupportDetails.capabilities.maxImageCount > 0 && imageCount > swapChainSupportDetails.capabilities.maxImageCount)
	{
		imageCount = swapChainSupportDetails.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR swapChainCreateInfo{};
	swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapChainCreateInfo.surface = mSurface;
	swapChainCreateInfo.minImageCount = imageCount;
	swapChainCreateInfo.imageFormat = swapFormat.format;
	swapChainCreateInfo.imageColorSpace = swapFormat.colorSpace;
	swapChainCreateInfo.imageExtent = swapExtents;
	swapChainCreateInfo.imageArrayLayers = 1;
	swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamiliesIndexStore queueFamiliesInfo = findSuitableQueueFamilies(mPhysicalDevice);
	uint32_t indeciesArr[] = {
		static_cast<uint32_t>(queueFamiliesInfo.graphicsFamalyIndex),
		static_cast<uint32_t>(queueFamiliesInfo.presentFamalyIndex)
	};

	if (queueFamiliesInfo.graphicsFamalyIndex != queueFamiliesInfo.presentFamalyIndex)
	{
		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapChainCreateInfo.queueFamilyIndexCount = 2;
		swapChainCreateInfo.pQueueFamilyIndices = indeciesArr;
	}
	else
	{
		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapChainCreateInfo.queueFamilyIndexCount = 0;
		swapChainCreateInfo.pQueueFamilyIndices = nullptr;
	}

	swapChainCreateInfo.preTransform = swapChainSupportDetails.capabilities.currentTransform;
	swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapChainCreateInfo.presentMode = swapPresentMode;
	swapChainCreateInfo.clipped = VK_TRUE;
	swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(mLogicalDevice, &swapChainCreateInfo, nullptr, &mSwapChain) != VK_SUCCESS)
	{
		std::cout << "\nFailed to create swap chain..." << std::endl;
		return false;
	}

	vkGetSwapchainImagesKHR(mLogicalDevice, mSwapChain, &imageCount, nullptr);
	mSwapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(mLogicalDevice, mSwapChain, &imageCount, mSwapChainImages.data());

	mSwapChainImageFormat = swapFormat.format;
	mSwapChainImageExtent = swapExtents;

	return true;
}

bool VulkanManager::createImageViews()
{
	int imageCount = mSwapChainImages.size();
	mSwapChainImageViews.resize(imageCount);

	for (int i = 0; i < imageCount; i++)
	{
		VkImageViewCreateInfo imageViewCreateInfo{};
		imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCreateInfo.image = mSwapChainImages[i];
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewCreateInfo.format = mSwapChainImageFormat;
		imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		imageViewCreateInfo.subresourceRange.levelCount = 1;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		imageViewCreateInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(mLogicalDevice, &imageViewCreateInfo, nullptr, &mSwapChainImageViews[i]) != VK_SUCCESS)
		{
			std::cout << "\nFailed to create image view..." << std::endl;
			return false;
		}
	}

	return true;
}

bool VulkanManager::createGraphicsPipeline()
{
	std::vector<char> basicTriVertShaderCode = mShaderManager.readShaderFile("../../../../Assets/Shaders/ByteEncoded/BasicTriangle_VS.spv");
	std::vector<char> basicTriFragShaderCode = mShaderManager.readShaderFile("../../../../Assets/Shaders/ByteEncoded/BasicTriangle_FS.spv");

	VkShaderModule basicTriVertShaderModule = mShaderManager.createShaderModule(mLogicalDevice, basicTriVertShaderCode);
	VkShaderModule basicTriFragShaderModule = mShaderManager.createShaderModule(mLogicalDevice, basicTriFragShaderCode);

	VkPipelineShaderStageCreateInfo vertShaderStageCreateInfo{};
	vertShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageCreateInfo.module = basicTriVertShaderModule;
	vertShaderStageCreateInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageCreateInfo{};
	fragShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageCreateInfo.module = basicTriFragShaderModule;
	fragShaderStageCreateInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageCreateInfo, fragShaderStageCreateInfo };

	std::vector<VkDynamicState> dynamicStates{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{};
	dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 0;
	vertexInputInfo.pVertexBindingDescriptions = nullptr;
	vertexInputInfo.vertexAttributeDescriptionCount = 0;
	vertexInputInfo.pVertexAttributeDescriptions = nullptr;

	VkPipelineInputAssemblyStateCreateInfo assemblyInputInfo{};
	assemblyInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	assemblyInputInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	assemblyInputInfo.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport{};
	viewport.x = 0;
	viewport.y = 0;
	viewport.width = (float)mSwapChainImageExtent.width;
	viewport.height = (float)mSwapChainImageExtent.height;
	viewport.minDepth = 0.0;
	viewport.maxDepth = 1.0;

	VkRect2D scissorRect{};
	scissorRect.offset = { 0, 0 };
	scissorRect.extent = mSwapChainImageExtent;

	VkPipelineViewportStateCreateInfo viewportCreateInfo{};
	viewportCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportCreateInfo.viewportCount = 1;
	viewportCreateInfo.scissorCount = 1;

	VkPipelineRasterizationStateCreateInfo rasterizationInfo{};
	rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationInfo.depthClampEnable = VK_FALSE;
	rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizationInfo.lineWidth = 1.0;
	rasterizationInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizationInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizationInfo.depthBiasEnable = VK_FALSE;
	rasterizationInfo.depthBiasConstantFactor = 0.0;
	rasterizationInfo.depthBiasClamp = 0.0;
	rasterizationInfo.depthBiasSlopeFactor = 0.0;

	VkPipelineMultisampleStateCreateInfo multisamplingInfo{};
	multisamplingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisamplingInfo.sampleShadingEnable = VK_FALSE;
	multisamplingInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisamplingInfo.minSampleShading = 1.0f;
	multisamplingInfo.pSampleMask = nullptr;
	multisamplingInfo.alphaToCoverageEnable = VK_FALSE;
	multisamplingInfo.alphaToOneEnable = VK_FALSE;

	//VkPipelineDepthStencilStateCreateInfo here

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
		| VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_TRUE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo blendStateCreateInfo{};
	blendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	blendStateCreateInfo.logicOpEnable = VK_TRUE;
	blendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY;
	blendStateCreateInfo.attachmentCount = 1;
	blendStateCreateInfo.pAttachments = &colorBlendAttachment;
	blendStateCreateInfo.blendConstants[0] = 0.0f;
	blendStateCreateInfo.blendConstants[1] = 0.0f;
	blendStateCreateInfo.blendConstants[2] = 0.0f;
	blendStateCreateInfo.blendConstants[3] = 0.0f;

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = 0;
	pipelineLayoutCreateInfo.pSetLayouts = nullptr;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

	if (vkCreatePipelineLayout(mLogicalDevice, &pipelineLayoutCreateInfo, nullptr, &mPipelineLayout) != VK_SUCCESS)
	{
		std::cout << "\nFailed to create pipeline layout..." << std::endl;
		return false;
	}

	VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo{};
	graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphicsPipelineCreateInfo.stageCount = 2;
	graphicsPipelineCreateInfo.pStages = shaderStages;
	graphicsPipelineCreateInfo.pVertexInputState = &vertexInputInfo;
	graphicsPipelineCreateInfo.pInputAssemblyState = &assemblyInputInfo;
	graphicsPipelineCreateInfo.pViewportState = &viewportCreateInfo;
	graphicsPipelineCreateInfo.pRasterizationState = &rasterizationInfo;
	graphicsPipelineCreateInfo.pMultisampleState = &multisamplingInfo;
	graphicsPipelineCreateInfo.pDepthStencilState = nullptr;
	graphicsPipelineCreateInfo.pColorBlendState = &blendStateCreateInfo;
	graphicsPipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
	graphicsPipelineCreateInfo.layout = mPipelineLayout;
	graphicsPipelineCreateInfo.renderPass = mRenderPass;
	graphicsPipelineCreateInfo.subpass = 0;
	graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
	graphicsPipelineCreateInfo.basePipelineIndex = -1;

	if (vkCreateGraphicsPipelines(mLogicalDevice, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &mGraphicsPipeline) != VK_SUCCESS)
	{
		std::cout << "\nFailed to create graphics pipeline..." << std::endl;
		return false;
	}

	vkDestroyShaderModule(mLogicalDevice, basicTriVertShaderModule, nullptr);
	vkDestroyShaderModule(mLogicalDevice, basicTriFragShaderModule, nullptr);

	return true;
}

bool VulkanManager::createRenderPass()
{
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = mSwapChainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subPassDescription{};
	subPassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subPassDescription.colorAttachmentCount = 1;
	subPassDescription.pColorAttachments = &colorAttachmentRef;

	VkRenderPassCreateInfo renderPassCreateInfo{};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount = 1;
	renderPassCreateInfo.pAttachments = &colorAttachment;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subPassDescription;

	if (vkCreateRenderPass(mLogicalDevice, &renderPassCreateInfo, nullptr, &mRenderPass) != VK_SUCCESS)
	{
		return false;
	}

	return true;
}
