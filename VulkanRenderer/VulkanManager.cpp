#include "VulkanManager.h"
#include "Helpers/BufferHelpers.h"

#include "UniformObjectHandlers//UniformObjects/UniformBufferDescriptor.h"
#include "UniformObjectHandlers/UniformObjects/UniformImageDescriptor.h"

bool VulkanManager::initVulkan(GLFWwindow* window)
{
	std::cout << "\nInitializing Vulkan" << std::endl;

	glfwSetWindowUserPointer(window, this);
	glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);

	createInstance();
	setUpDebugMessenger();
	createSurface(window);
	pickPhysicalDevice();
	createLogicalDevice();
	createSwapChain(window);
	createImageViews();

	createDepthResources();
	createMSAA_ColorResources();

	if (!mPipelineData.dynamicRenderingEnabled())
	{
		mPipelineData.createRenderPass(mLogicalDevice, mSwapChainImageFormat, mDepthFormat, mMSAA_Samples);
		mPipelineData.createFramebuffers(mLogicalDevice, mSwapChainImageViews, mDepthImageView, mMSAA_ColorImageView, 
			mSwapChainImageExtent);
	}

	createCommandPool();

	uint32_t texMipLevels;
	createTextureImage(mLogicalDevice, mPhysicalDevice, mTextureImage, mTextureMemory, mCommandPool, mGraphicsQueue,
		"../Assets/Models/Room/room.png", texMipLevels);

	if (!createImageView(mLogicalDevice, mTextureImage, &mTextureImageView, VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_ASPECT_COLOR_BIT, texMipLevels) != VK_SUCCESS)
	{
		std::cout << "\nFailed to create texture image view..." << std::endl;
		return false;
	}

	createTextureSampler(mLogicalDevice, mPhysicalDevice, &mTextureSampler);

	DescriptorPoolCreateData descPoolCreateInfo{};
	descPoolCreateInfo.maxDescriptorSets = 2;
	descPoolCreateInfo.maxFramesInFlight = MAX_FRAMES_IN_FLIGHT;
	descPoolCreateInfo.uniformBufferCount = 2;
	descPoolCreateInfo.storageBufferCount = 2;
	descPoolCreateInfo.combinedImageSamplerCount = 1;

	mDescriptorPool.createDescriptorPool(mLogicalDevice, descPoolCreateInfo);

	DescriptorSetsData descriptorData{};
	std::vector<UniformBufferDescriptor*> descriptos;

	des1.setDstBinding(0);
	VkDescriptorBufferInfo bufferInfo{};
	bufferInfo.offset = 0;
	bufferInfo.range = sizeof(ModelViewProjectionUniformObject);
	des1.setBufferInfo(bufferInfo);
	descriptos.push_back(&des1);

	std::vector<UniformImageDescriptor*> descriptos1;
	VkDescriptorImageInfo imageInfo{};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView = mTextureImageView;
	imageInfo.sampler = mTextureSampler;

	std::vector<UniformBufferDescriptor*> descriptos2;

	des3 = UniformBufferDescriptor(0, VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, sizeof(DeltaTimeUniformObject));
	descriptos2.push_back(&des3);

	des4 = UniformBufferDescriptor(1, VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 
		sizeof(Particle2D) * PARTICLE_COUNT, nullptr, true);
	descriptos2.push_back(&des4);

	des5 = UniformBufferDescriptor(2, VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		sizeof(Particle2D) * PARTICLE_COUNT);
	descriptos2.push_back(&des5);

	std::vector<UniformImageDescriptor*> descriptos3;
	imageInfo = {};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView = mTextureImageView;
	imageInfo.sampler = mTextureSampler;

	des2.setImageInfo(imageInfo);
	des2.setDstBinding(1);
	descriptos1.push_back(&des2);

	descriptorData.loadDescriptorSets(descriptos, descriptos1, descriptos2, descriptos3, MAX_FRAMES_IN_FLIGHT);
	mUniformBufferData.setDescriptorSetData(descriptorData);

	mUniformBufferData.createDescriptorSetLayout(mLogicalDevice);
	mUniformBufferData.createComputeDescriptorSetLayout(mLogicalDevice);

	mVertexBufferData.createVertexDataFromModel(mLogicalDevice, mPhysicalDevice, mCommandPool, mGraphicsQueue, "../Assets/Models/Room/room.obj");

	ConfigurablePipelineValues configValues{};
	configValues.samples = mMSAA_Samples;
	configValues.primitiveTopology = mRenderingParticles ? VK_PRIMITIVE_TOPOLOGY_POINT_LIST : VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	configValues.depthWriteEnabled = !mRenderingParticles;

	const char* vertShader = mRenderingParticles ? "../Assets/Shaders/ByteEncoded/RenderParticles_VS.spv" : "../Assets/Shaders/ByteEncoded/BasicTriangle_VS.spv";
	const char* fragShader = mRenderingParticles ? "../Assets/Shaders/ByteEncoded/RenderParticles_FS.spv" : "../Assets/Shaders/ByteEncoded/BasicTriangle_FS.spv";

	VertexInputData vertexInputInfo = mRenderingParticles ? Particle2D::getParticleInputData() : mVertexBufferData.getVertexInputData();

	mPipelineData.createGraphicsPipeline(mLogicalDevice, mSwapChainImageExtent, mSwapChainImageFormat, mDepthFormat,
		mUniformBufferData.getDescriptorSetLayout(), configValues, vertShader, fragShader, vertexInputInfo);
	mPipelineData.createComputePipeline(mLogicalDevice, mUniformBufferData.getComputeDescriptorSetLayout());

	mUniformBufferData.createSSBOs(mLogicalDevice, mPhysicalDevice, MAX_FRAMES_IN_FLIGHT, PARTICLE_COUNT,
		mSwapChainImageExtent.height / (float)mSwapChainImageExtent.width, mCommandPool, mGraphicsQueue);

	mUniformBufferData.createUniformBuffers(mLogicalDevice, mPhysicalDevice, MAX_FRAMES_IN_FLIGHT);

	mUniformBufferData.createDescriptorSetsData(mLogicalDevice, mDescriptorPool.getDescriptorPool(), MAX_FRAMES_IN_FLIGHT);
	mUniformBufferData.createComputeDescriptorSets(mLogicalDevice, mDescriptorPool.getDescriptorPool(), MAX_FRAMES_IN_FLIGHT, 
		PARTICLE_COUNT);

	createCommandBuffers();
	createSyncObjects();
	registerExtensionFunctions(mInstance);

	mSelectedScene = mScenes[0];

	return false;
}

bool VulkanManager::cleanupVulkan()
{
	//Waits for rendering semaphores to finish
	vkDeviceWaitIdle(mLogicalDevice);

	int numSwapChainImages = mSwapChainImages.size();

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkDestroySemaphore(mLogicalDevice, mImageAvailableSemaphores[i], nullptr);
		vkDestroyFence(mLogicalDevice, mWhileRenderingFences[i], nullptr);

		vkDestroySemaphore(mLogicalDevice, mComputeFinishedSemaphores[i], nullptr);
		vkDestroyFence(mLogicalDevice, mWhileComputingFences[i], nullptr);
	}

	for (size_t i = 0; i < numSwapChainImages; i++)
	{
		vkDestroySemaphore(mLogicalDevice, mRenderFinishedSemaphores[i], nullptr);
	}

	vkDestroyCommandPool(mLogicalDevice, mCommandPool, nullptr);

	cleanupSwapChain();

	vkDestroySampler(mLogicalDevice, mTextureSampler, nullptr);
	vkDestroyImageView(mLogicalDevice, mTextureImageView, nullptr);
	vkDestroyImage(mLogicalDevice, mTextureImage, nullptr);
	vkFreeMemory(mLogicalDevice, mTextureMemory, nullptr);

	vkDestroyImageView(mLogicalDevice, mDepthImageView, nullptr);
	vkDestroyImage(mLogicalDevice, mDepthImage, nullptr);
	vkFreeMemory(mLogicalDevice, mDepthMemory, nullptr);

	vkDestroyImageView(mLogicalDevice, mMSAA_ColorImageView, nullptr);
	vkDestroyImage(mLogicalDevice, mMSAA_ColorImage, nullptr);
	vkFreeMemory(mLogicalDevice, mMSAA_ColorhMemory, nullptr);

	mUniformBufferData.cleanup(mLogicalDevice);
	mDescriptorPool.cleanup(mLogicalDevice);
	mVertexBufferData.cleanupBuffers(mLogicalDevice);
	mPipelineData.cleanupPipeline(mLogicalDevice);

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

bool VulkanManager::drawFrame(GLFWwindow* window, float dt, bool* needToReloadGUI_Flag)
{
	VkSubmitInfo submitInfo{};

	//CPU waits until fence has been signaled by GPU (compute done from previous frame)
	vkWaitForFences(mLogicalDevice, 1, &mWhileComputingFences[mCurrentFrame], VK_TRUE, UINT64_MAX);

	mUniformBufferData.updateUniformBuffers(mCurrentFrame, mSwapChainImageExtent.width / (float)mSwapChainImageExtent.height, dt);

	//Only run compute shader when actually rendering the particles
	if (mRenderingParticles)
	{
		vkResetFences(mLogicalDevice, 1, &mWhileComputingFences[mCurrentFrame]);
		vkResetCommandBuffer(mComputeCommandBuffers[mCurrentFrame], 0);

		recordComputeCommandBuffer(mComputeCommandBuffers[mCurrentFrame]);

		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &mComputeCommandBuffers[mCurrentFrame];
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &mComputeFinishedSemaphores[mCurrentFrame];

		if (vkQueueSubmit(mComputeQueue, 1, &submitInfo, mWhileComputingFences[mCurrentFrame]) != VK_SUCCESS)
		{
			std::cout << "Failed to submit command buffer to compute queue..." << std::endl;
			return false;
		}
	}

	//CPU waits until fence has been signaled by GPU (rendering done from previous frame)
	vkWaitForFences(mLogicalDevice, 1, &mWhileRenderingFences[mCurrentFrame], VK_TRUE, UINT64_MAX);

	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(mLogicalDevice, mSwapChain, UINT64_MAX, mImageAvailableSemaphores[mCurrentFrame], VK_NULL_HANDLE, &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		//TODO: Maybe just store the window pointer as a member to avoid func parameter here
		recreateSwapChain(window);
		return true;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		std::cout << "Failed to aquire next swapchain image..." << std::endl;
		return false;
	}

	//Resets fence only if image has been aquired
	vkResetFences(mLogicalDevice, 1, &mWhileRenderingFences[mCurrentFrame]);
	vkResetCommandBuffer(mGraphicsCommandBuffers[mCurrentFrame], 0);

	if (mPipelineData.dynamicRenderingEnabled()) 
	{
		renderScene_DynamicRendering(mGraphicsCommandBuffers[mCurrentFrame], imageIndex);
	}
	else 
	{
		renderScene(mGraphicsCommandBuffers[mCurrentFrame], imageIndex);
	}

	//Only adds the compute shader wait semephore if actually rendering the particles which depend on the compute shader
	std::vector<VkSemaphore> waitSemaphores = { mImageAvailableSemaphores[mCurrentFrame]};
	if (mRenderingParticles)
	{
		waitSemaphores.push_back(mComputeFinishedSemaphores[mCurrentFrame]);
	}

	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
	submitInfo.pWaitSemaphores = waitSemaphores.data();
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &mGraphicsCommandBuffers[mCurrentFrame];
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &mRenderFinishedSemaphores[imageIndex];

	if (vkQueueSubmit(mGraphicsQueue, 1, &submitInfo, mWhileRenderingFences[mCurrentFrame]) != VK_SUCCESS)
	{
		std::cout << "Failed to submit command buffer to graphics queue..." << std::endl;
	}

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &mRenderFinishedSemaphores[imageIndex];

	VkSwapchainKHR swapChains[] = { mSwapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr;

	result = vkQueuePresentKHR(mPresentQueue, &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || mFramebuffersResized)
	{
		//TODO: Maybe just store the window pointer as a member to avoid func parameter here
		recreateSwapChain(window);
		mFramebuffersResized = false;
		return true;
	}
	else if (result != VK_SUCCESS)
	{
		std::cout << "Failed to present swap chain image..." << std::endl;
		return false;
	}

	handlePipelineChanges(window, needToReloadGUI_Flag);

	mCurrentFrame = (mCurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

	return true;
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
		std::cout << "\nFailed to find vkCreateDebugUtilsMessengerEXT..." << std::endl;
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

void VulkanManager::registerExtensionFunctions(VkInstance instance)
{
	fpCmdBeginRenderingKHR = (PFN_vkCmdBeginRenderingKHR)vkGetInstanceProcAddr(instance, "vkCmdBeginRenderingKHR");
	fpCmdEndRenderingKHR = (PFN_vkCmdEndRenderingKHR)vkGetInstanceProcAddr(instance, "vkCmdEndRenderingKHR");
	fpCmdPipelineBarrier2 = (PFN_vkCmdPipelineBarrier2KHR)vkGetInstanceProcAddr(instance, "vkCmdPipelineBarrier2KHR");
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
			mMSAA_Samples = getMaxUsableSampleCount();
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
		//Not using an asyncronous compute queue
		if ((queueProperty.queueFlags & VK_QUEUE_GRAPHICS_BIT) && (queueProperty.queueFlags & VK_QUEUE_COMPUTE_BIT))
		{
			queueIndexInfo.graphicsFamalyIndex = i;
			queueIndexInfo.computeFamalyIndex = i;
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
	deviceVulkan13Features.dynamicRendering = VK_TRUE;
	deviceVulkan13Features.synchronization2 = VK_TRUE;

	deviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	deviceFeatures.pNext = (VkPhysicalDeviceVulkan13Features*)&deviceVulkan13Features;

	vkGetPhysicalDeviceFeatures2(device, &deviceFeatures);

	return deviceVulkan13Features.synchronization2 && deviceVulkan13Features.dynamicRendering && 
		deviceExtendedStateFeatures.extendedDynamicState && deviceFeatures.features.samplerAnisotropy;
}

VkFormat VulkanManager::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
	for (VkFormat format : candidates)
	{
		VkFormatProperties properties{};
		vkGetPhysicalDeviceFormatProperties(mPhysicalDevice, format, &properties);

		if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features)
		{
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features)
		{
			return format;
		}
	}

	throw std::runtime_error("Could not find supported format...");
}

VkFormat VulkanManager::findDepthFormat()
{
	return findSupportedFormat({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

bool VulkanManager::hasStencilComponent(VkFormat format)
{
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
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
	deviceBaseFeatures.samplerAnisotropy = VK_TRUE;

	VkPhysicalDeviceFeatures2 deviceAdditFeatures{};
	VkPhysicalDeviceVulkan13Features deviceVulkan13Features{};
	VkPhysicalDeviceExtendedDynamicStateFeaturesEXT deviceExtendedStateFeatures{};

	deviceExtendedStateFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT;

	deviceVulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
	deviceVulkan13Features.pNext = (VkPhysicalDeviceDynamicRenderingFeatures*)&deviceExtendedStateFeatures;
	deviceVulkan13Features.dynamicRendering = VK_TRUE;
	deviceVulkan13Features.synchronization2 = VK_TRUE;

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
	vkGetDeviceQueue(mLogicalDevice, queueFamilyIndeciesInfo.computeFamalyIndex, 0, &mComputeQueue);
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

	imageCount = swapChainSupportDetails.capabilities.minImageCount;

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

bool VulkanManager::recreateSwapChain(GLFWwindow* window)
{
	int width, height = 0;
	glfwGetFramebufferSize(window, &width, &height);
	
	//Stalls until window is valid/maximized
	while (width == 0 || height == 0)
	{
		glfwGetFramebufferSize(window, &width, &height);
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(mLogicalDevice);

	cleanupSwapChain();

	vkDestroyImageView(mLogicalDevice, mDepthImageView, nullptr);
	vkDestroyImage(mLogicalDevice, mDepthImage, nullptr);
	vkFreeMemory(mLogicalDevice, mDepthMemory, nullptr);

	vkDestroyImageView(mLogicalDevice, mMSAA_ColorImageView, nullptr);
	vkDestroyImage(mLogicalDevice, mMSAA_ColorImage, nullptr);
	vkFreeMemory(mLogicalDevice, mMSAA_ColorhMemory, nullptr);

	createSwapChain(window);
	createImageViews();
	createMSAA_ColorResources();
	createDepthResources();

	if (!mPipelineData.dynamicRenderingEnabled())
	{
		mPipelineData.createFramebuffers(mLogicalDevice, mSwapChainImageViews, mDepthImageView, mMSAA_ColorImageView, mSwapChainImageExtent);
	}

	return false;
}

void VulkanManager::cleanupSwapChain()
{
	mPipelineData.cleanupFrambuffers(mLogicalDevice);

	for (VkImageView imageView : mSwapChainImageViews)
	{
		vkDestroyImageView(mLogicalDevice, imageView, nullptr);
	}

	vkDestroySwapchainKHR(mLogicalDevice, mSwapChain, nullptr);
}

bool VulkanManager::createImageViews()
{
	int imageCount = mSwapChainImages.size();
	mSwapChainImageViews.resize(imageCount);

	for (int i = 0; i < imageCount; i++)
	{
		if (!createImageView(mLogicalDevice, mSwapChainImages[i], &mSwapChainImageViews[i], mSwapChainImageFormat, 
			VK_IMAGE_ASPECT_COLOR_BIT, 1))
		{
			std::cout << "\nFailed to create swap chain image view..." << std::endl;
			return false;
		}
	}

	return true;
}

bool VulkanManager::createDepthResources()
{
	mDepthFormat = findDepthFormat();

	if (!createImage(mLogicalDevice, mPhysicalDevice, mSwapChainImageExtent.width, mSwapChainImageExtent.height, 1, mMSAA_Samples, 
		mDepthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		mDepthImage, mDepthMemory)) { return false; }

	if (!createImageView(mLogicalDevice, mDepthImage, &mDepthImageView, mDepthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1)) { return false; }

	return true;
}

bool VulkanManager::createMSAA_ColorResources()
{
	if (!createImage(mLogicalDevice, mPhysicalDevice, mSwapChainImageExtent.width, mSwapChainImageExtent.height, 1, 
		mMSAA_Samples, mSwapChainImageFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | 
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mMSAA_ColorImage, mMSAA_ColorhMemory)) 
		{ return false; }

	if (!createImageView(mLogicalDevice, mMSAA_ColorImage, &mMSAA_ColorImageView, mSwapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1)) 
	{ return false; }

	return true;
}

bool VulkanManager::createCommandPool()
{
	QueueFamiliesIndexStore queueFamilies = findSuitableQueueFamilies(mPhysicalDevice);

	VkCommandPoolCreateInfo poolCreateInfo{};
	poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolCreateInfo.queueFamilyIndex = queueFamilies.graphicsFamalyIndex;

	if (vkCreateCommandPool(mLogicalDevice, &poolCreateInfo, nullptr, &mCommandPool) != VK_SUCCESS)
	{
		std::cout << "\nFailed to create command pool..." << std::endl;
		return false;
	}

	return true;
}

bool VulkanManager::createCommandBuffers()
{
	mGraphicsCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

	VkCommandBufferAllocateInfo commandBufAllocateInfo{};
	commandBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufAllocateInfo.commandPool = mCommandPool;
	commandBufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufAllocateInfo.commandBufferCount = (uint32_t)MAX_FRAMES_IN_FLIGHT;

	if (vkAllocateCommandBuffers(mLogicalDevice, &commandBufAllocateInfo, mGraphicsCommandBuffers.data()) != VK_SUCCESS)
	{
		std::cout << "\nFailed to allocate graphics command buffers..." << std::endl;
		return false;
	}

	mComputeCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

	commandBufAllocateInfo = {};
	commandBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufAllocateInfo.commandPool = mCommandPool;
	commandBufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufAllocateInfo.commandBufferCount = (uint32_t)MAX_FRAMES_IN_FLIGHT;

	if (vkAllocateCommandBuffers(mLogicalDevice, &commandBufAllocateInfo, mComputeCommandBuffers.data()) != VK_SUCCESS)
	{
		std::cout << "\nFailed to allocate compute command buffers..." << std::endl;
		return false;
	}

	return true;
}

bool VulkanManager::renderScene(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
	VkCommandBufferBeginInfo beginCommandBuffInfo{};
	beginCommandBuffInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginCommandBuffInfo.flags = 0;
	beginCommandBuffInfo.pInheritanceInfo = nullptr;

	if (vkBeginCommandBuffer(commandBuffer, &beginCommandBuffInfo) != VK_SUCCESS)
	{
		std::cout << "\nFailed to begin recording command buffer..." << std::endl;
		return false;
	}
	
	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
	clearValues[1].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo renderPassBeginInfo = mPipelineData.getRenderPassBeginInfo(imageIndex, mSwapChainImageExtent, 
		clearValues);

	vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineData.getGraphicsPipeline());

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(mSwapChainImageExtent.width);
	viewport.height = static_cast<float>(mSwapChainImageExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = mSwapChainImageExtent;

	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	mUniformBufferData.bindGraphicsDescriptorSets(commandBuffer, mPipelineData.getGraphicsPipelineLayout(), mCurrentFrame);
	
	if (mRenderingParticles)
	{
		mUniformBufferData.bindSSBOs(commandBuffer, mCurrentFrame, PARTICLE_COUNT);
	}
	else 
	{
		mVertexBufferData.draw(commandBuffer);
	}

	renderGUI(commandBuffer, imageIndex);

	vkCmdEndRenderPass(commandBuffer);

	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
	{
		std::cout << "\nFailed to end command buffer recording..." << std::endl;
		return false;
	}

	return true;
}

bool VulkanManager::renderScene_DynamicRendering(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
	VkCommandBufferBeginInfo beginCommandBuffInfo{};
	beginCommandBuffInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginCommandBuffInfo.flags = 0;
	beginCommandBuffInfo.pInheritanceInfo = nullptr;

	if (vkBeginCommandBuffer(commandBuffer, &beginCommandBuffInfo) != VK_SUCCESS)
	{
		std::cout << "\nFailed to begin recording command buffer..." << std::endl;
		return false;
	}

	//Transitions swap chain correct formats, accesses, and stages for rendering (as resolve attachment for msaa)
	transitionImageLayout(commandBuffer, mSwapChainImages[imageIndex], VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_2_NONE, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_NONE,
		VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_IMAGE_ASPECT_COLOR_BIT, 1, fpCmdPipelineBarrier2);
	
	//Transitions multisampling color attachment formats, accesses, and stages for multisample rendering
	transitionImageLayout(commandBuffer, mMSAA_ColorImage, VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_2_NONE, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT | 
		VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT, VK_PIPELINE_STAGE_2_NONE, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, 
		VK_IMAGE_ASPECT_COLOR_BIT, 1, fpCmdPipelineBarrier2);

	//Transitions depth attachment formats, accesses, and stages for multisample rendering
	transitionImageLayout(commandBuffer, mDepthImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, 
		VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
		VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT, 
		VK_IMAGE_ASPECT_DEPTH_BIT, 1, fpCmdPipelineBarrier2);

	VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
	VkClearValue clearDepth = { 1.0f, 0 };
	VkRenderingAttachmentInfoKHR colorAttachmentInfo{};
	colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
	colorAttachmentInfo.clearValue = clearColor;
	colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachmentInfo.imageView = mSwapChainImageViews[imageIndex];

	//Adds resolve image information if using MSAA
	if (mMSAA_Samples != VK_SAMPLE_COUNT_1_BIT)
	{
		colorAttachmentInfo.resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
		colorAttachmentInfo.resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		colorAttachmentInfo.resolveImageView = mSwapChainImageViews[imageIndex];
		colorAttachmentInfo.imageView = mMSAA_ColorImageView;
	}

	VkRenderingAttachmentInfoKHR depthAttachmentInfo{};
	depthAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
	depthAttachmentInfo.clearValue = clearDepth;
	depthAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	depthAttachmentInfo.resolveImageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
	depthAttachmentInfo.imageView = mDepthImageView;

	//std::array<VkRenderingAttachmentInfoKHR, 1> colorAttachments = { colorAttachmentInfo };

	VkRenderingInfoKHR renderingInfo{};
	renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
	renderingInfo.colorAttachmentCount = 1;
	renderingInfo.pColorAttachments = &colorAttachmentInfo;
	renderingInfo.renderArea.offset = { 0, 0 };
	renderingInfo.renderArea.extent = mSwapChainImageExtent;
	renderingInfo.layerCount = 1;
	renderingInfo.pDepthAttachment = &depthAttachmentInfo;

	fpCmdBeginRenderingKHR(commandBuffer, &renderingInfo);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineData.getGraphicsPipeline());

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(mSwapChainImageExtent.width);
	viewport.height = static_cast<float>(mSwapChainImageExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = mSwapChainImageExtent;

	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	mUniformBufferData.bindGraphicsDescriptorSets(commandBuffer, mPipelineData.getGraphicsPipelineLayout(), mCurrentFrame);
	
	if (mRenderingParticles)
	{
		mUniformBufferData.bindSSBOs(commandBuffer, mCurrentFrame, PARTICLE_COUNT);
	}
	else 
	{
		mVertexBufferData.draw(commandBuffer);
	}

	fpCmdEndRenderingKHR(commandBuffer);

	renderGUI_DynamicRender(mGraphicsCommandBuffers[mCurrentFrame], imageIndex);

	//Transitions swap chain image to the correct formats, accesses, and stages for presenting
	transitionImageLayout(commandBuffer, mSwapChainImages[imageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_2_NONE,
		VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
		VK_IMAGE_ASPECT_COLOR_BIT, 1, fpCmdPipelineBarrier2);

	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
	{
		std::cout << "\nFailed to end command buffer recording..." << std::endl;
		return false;
	}

	return false;
}

void VulkanManager::handlePipelineChanges(GLFWwindow* window, bool* needToReloadGUI_Flag)
{
	if (mRemakePipelineTriggered)
	{
		//Waits for device to finish up before recreating pipeline
		vkDeviceWaitIdle(mLogicalDevice);

		mRenderingParticles = mSelectedScene.sceneType == PARTICLES;

		vkDestroyPipeline(mLogicalDevice, mPipelineData.getGraphicsPipeline(), nullptr);
		vkDestroyPipelineLayout(mLogicalDevice, mPipelineData.getGraphicsPipelineLayout(), nullptr);

		ConfigurablePipelineValues configValues{};
		configValues.samples = mMSAA_Samples;
		configValues.primitiveTopology = mRenderingParticles ? VK_PRIMITIVE_TOPOLOGY_POINT_LIST : VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		configValues.depthWriteEnabled = !mRenderingParticles;

		const char* vertShader = mRenderingParticles ? "../Assets/Shaders/ByteEncoded/RenderParticles_VS.spv" : "../Assets/Shaders/ByteEncoded/BasicTriangle_VS.spv";
		const char* fragShader = mRenderingParticles ? "../Assets/Shaders/ByteEncoded/RenderParticles_FS.spv" : "../Assets/Shaders/ByteEncoded/BasicTriangle_FS.spv";

		VertexInputData vertexInputInfo = mRenderingParticles ? Particle2D::getParticleInputData() : mVertexBufferData.getVertexInputData();
		mPipelineData.createGraphicsPipeline(mLogicalDevice, mSwapChainImageExtent, mSwapChainImageFormat, mDepthFormat,
			mUniformBufferData.getDescriptorSetLayout(), configValues, vertShader, fragShader, vertexInputInfo);

		mRemakePipelineTriggered = false;
	}

	if (mSwitchingRenderMethod)
	{
		//Waits for device to finish up before recreating pipeline
		vkDeviceWaitIdle(mLogicalDevice);

		vkDestroyPipeline(mLogicalDevice, mPipelineData.getGraphicsPipeline(), nullptr);
		vkDestroyPipelineLayout(mLogicalDevice, mPipelineData.getGraphicsPipelineLayout(), nullptr);

		mPipelineData.setDynamicRenderingEnabled(mUsingDynamicRenderingForGUI);

		//TODO: Add function in pipeline to switch from dynamic rfendering
		if (!mUsingDynamicRenderingForGUI)
		{
			if (mPipelineData.getRenderPass() == VK_NULL_HANDLE)
			{
				mPipelineData.createRenderPass(mLogicalDevice, mSwapChainImageFormat, mDepthFormat, mMSAA_Samples);
			}

			if (mPipelineData.noLoadedFramebuffers())
			{
				mPipelineData.createFramebuffers(mLogicalDevice, mSwapChainImageViews, mDepthImageView, mMSAA_ColorImageView,
					mSwapChainImageExtent);
			}
		}

		ConfigurablePipelineValues configValues{};
		configValues.samples = mMSAA_Samples;
		configValues.primitiveTopology = mRenderingParticles ? VK_PRIMITIVE_TOPOLOGY_POINT_LIST : VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		configValues.depthWriteEnabled = !mRenderingParticles;

		const char* vertShader = mRenderingParticles ? "../Assets/Shaders/ByteEncoded/RenderParticles_VS.spv" : "../Assets/Shaders/ByteEncoded/BasicTriangle_VS.spv";
		const char* fragShader = mRenderingParticles ? "../Assets/Shaders/ByteEncoded/RenderParticles_FS.spv" : "../Assets/Shaders/ByteEncoded/BasicTriangle_FS.spv";

		VertexInputData vertexInputInfo = mRenderingParticles ? Particle2D::getParticleInputData() : mVertexBufferData.getVertexInputData();

		mPipelineData.createGraphicsPipeline(mLogicalDevice, mSwapChainImageExtent, mSwapChainImageFormat, mDepthFormat,
			mUniformBufferData.getDescriptorSetLayout(), configValues, vertShader, fragShader, vertexInputInfo);

		mSwitchingRenderMethod = false;

		if (needToReloadGUI_Flag != nullptr)
		{
			*needToReloadGUI_Flag = true;
		}
	}
}

bool VulkanManager::recordComputeCommandBuffer(VkCommandBuffer commandBuffer)
{
	VkCommandBufferBeginInfo beginCommandBuffInfo{};
	beginCommandBuffInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	if (vkBeginCommandBuffer(commandBuffer, &beginCommandBuffInfo) != VK_SUCCESS)
	{
		std::cout << "\nFailed to begin recording compute command buffer..." << std::endl;
		return false;
	}

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mPipelineData.getComputePipeline());
	mUniformBufferData.bindComputeDescriptorSets(commandBuffer, mPipelineData.getComputePipelineLayout(), mCurrentFrame);

	vkCmdDispatch(commandBuffer, PARTICLE_COUNT / 256, 1, 1);

	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
	{
		std::cout << "\nFailed to end compute command buffer recording..." << std::endl;
		return false;
	}

	return true;
}

bool VulkanManager::createSyncObjects()
{
	int numSwapChainImages = mSwapChainImages.size();

	mImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	mWhileRenderingFences.resize(MAX_FRAMES_IN_FLIGHT);

	mComputeFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	mWhileComputingFences.resize(MAX_FRAMES_IN_FLIGHT);

	//This is to stop unsafe reusage of semaphores: https://docs.vulkan.org/guide/latest/swapchain_semaphore_reuse.html
	mRenderFinishedSemaphores.resize(numSwapChainImages);

	VkSemaphoreCreateInfo semaphoreCreateInfo{};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceCreateInfo{};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; //Makes it so first drawFrame does not block

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		if (vkCreateSemaphore(mLogicalDevice, &semaphoreCreateInfo, nullptr, &mImageAvailableSemaphores[i]) != VK_SUCCESS
			|| vkCreateFence(mLogicalDevice, &fenceCreateInfo, nullptr, &mWhileRenderingFences[i]) != VK_SUCCESS)
		{
			std::cout << "\nFailed to create semaphores or fence..." << std::endl;
			return false;
		}

		if (vkCreateSemaphore(mLogicalDevice, &semaphoreCreateInfo, nullptr, &mComputeFinishedSemaphores[i]) != VK_SUCCESS
			|| vkCreateFence(mLogicalDevice, &fenceCreateInfo, nullptr, &mWhileComputingFences[i]) != VK_SUCCESS)
		{
			std::cout << "\nFailed to create semaphores or fence..." << std::endl;
			return false;
		}
	}

	for (size_t i = 0; i < numSwapChainImages; i++)
	{
		if (vkCreateSemaphore(mLogicalDevice, &semaphoreCreateInfo, nullptr, &mRenderFinishedSemaphores[i]) != VK_SUCCESS)
		{
			std::cout << "\nFailed to create semaphores or fence..." << std::endl;
			return false;
		}
	}



	return true;
}

void VulkanManager::renderGUI_DynamicRender(VkCommandBuffer commandBuffer, int imageIndex)
{
	updateGUI();

	VkCommandBufferBeginInfo beginCommandBuffInfo{};
	beginCommandBuffInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginCommandBuffInfo.flags = 0;
	beginCommandBuffInfo.pInheritanceInfo = nullptr;

	VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
	VkRenderingAttachmentInfoKHR colorAttachmentInfo{};
	colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
	colorAttachmentInfo.clearValue = clearColor;
	colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	colorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachmentInfo.imageView = mSwapChainImageViews[imageIndex];

	//Adds resolve image information if using MSAA
	if (mMSAA_Samples != VK_SAMPLE_COUNT_1_BIT)
	{
		colorAttachmentInfo.resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
		colorAttachmentInfo.resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		colorAttachmentInfo.resolveImageView = mSwapChainImageViews[imageIndex];
		colorAttachmentInfo.imageView = mMSAA_ColorImageView;
	}

	//std::array<VkRenderingAttachmentInfoKHR, 1> colorAttachments = { colorAttachmentInfo };

	VkRenderingInfoKHR renderingInfo{};
	renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
	renderingInfo.colorAttachmentCount = 1;
	renderingInfo.pColorAttachments = &colorAttachmentInfo;
	renderingInfo.renderArea.offset = { 0, 0 };
	renderingInfo.renderArea.extent = mSwapChainImageExtent;
	renderingInfo.layerCount = 1;

	fpCmdBeginRenderingKHR(commandBuffer, &renderingInfo);

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer, VK_NULL_HANDLE);

	fpCmdEndRenderingKHR(commandBuffer);
}

VkSampleCountFlagBits VulkanManager::getMaxUsableSampleCount()
{
	VkPhysicalDeviceProperties props{};
	vkGetPhysicalDeviceProperties(mPhysicalDevice, &props);

	VkSampleCountFlags count = props.limits.framebufferColorSampleCounts & props.limits.sampledImageDepthSampleCounts;

	if (count & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
	if (count & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
	if (count & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
	if (count & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
	if (count & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
	if (count & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

	return VK_SAMPLE_COUNT_1_BIT;
}

void VulkanManager::markFramebuffersResized()
{
	mFramebuffersResized = true;
}

void VulkanManager::updateGUI()
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();

	ImGui::NewFrame();

	ImGui::ShowDemoWindow();

	ImGui::Begin("DEBUG");

	if (ImGui::BeginCombo("Select Scene", mSelectedScene.name.c_str()))
	{
		for (SceneData scene : mScenes)
		{
			if (ImGui::Selectable(scene.name.c_str(), &scene.selected))
			{
				mSelectedScene = scene;
				mRemakePipelineTriggered = true;
			}
		}
		ImGui::EndCombo();
	}

	if (ImGui::Checkbox("Using Dynamic Rendering", &mUsingDynamicRenderingForGUI))
	{
		mSwitchingRenderMethod = true;
	}

	ImGui::End();

	ImGui::Render();
}

void VulkanManager::renderGUI(VkCommandBuffer commandBuffer, int imageIndex)
{
	updateGUI();
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer, VK_NULL_HANDLE);
}
