#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include "vulkan/vulkan.h"

#include <iostream>
#include <vector>

#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else 
	const bool enableValidationLayers = true;
#endif

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData) {
	std::cerr << "Validation Layer: " << pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}

class VulkanManager {
public:
	bool initVulkan();
	bool cleanupVulkan();
private:
	bool createInstance();
	bool hasValidationLayerSupport();
	std::vector<const char*> getRequiredExtensions();
	bool setUpDebugMessenger();
	void populateDebugMessangerInfo(VkDebugUtilsMessengerCreateInfoEXT& debugMessengerInfo);

	VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
		const VkAllocationCallbacks* pAlloator, VkDebugUtilsMessengerEXT* pDebugMessenger);
	void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
		const VkAllocationCallbacks* pAllocator);

	bool pickPhysicalDevice();
	bool deviceIsSuitable(VkPhysicalDevice device);
	int findSuitableQueueFamilies(VkPhysicalDevice device);
	bool supportsDeviceExtensions(VkPhysicalDevice device);
	bool supportsDeviceFeatures(VkPhysicalDevice device);

	bool createLogicalDevice();

	VkInstance mInstance;
	VkDebugUtilsMessengerEXT mDebugMessanger;
	VkPhysicalDevice mPhysicalDevice = VK_NULL_HANDLE;
	VkDevice mLogicalDevice;
	VkQueue mGraphicsQueue;

	const std::vector<const char*> mValidationLayers = {
	"VK_LAYER_KHRONOS_validation"
	};

	std::vector<const char*> mRequiredDeviceExtension = { 
		VK_KHR_SWAPCHAIN_EXTENSION_NAME 
	};
};
