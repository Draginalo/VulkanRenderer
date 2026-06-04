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
#include <set>
#include <algorithm>

#include "GraphicsPipeline.h"
#include "VertexBufferData.h"
#include "UniformBufferData.h"

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

struct QueueFamiliesIndexStore {
	int graphicsFamalyIndex = -1;
	int presentFamalyIndex = -1;

	bool containsAllFamilies()
	{
		return graphicsFamalyIndex != -1 && presentFamalyIndex != -1;
	}

	std::set<uint32_t> getVectorOfIndecies()
	{
		return std::set<uint32_t> {
			static_cast<uint32_t>(graphicsFamalyIndex),
			static_cast<uint32_t>(presentFamalyIndex)
		};
	}
};

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

class VulkanManager {
public:
	bool initVulkan(GLFWwindow* window);
	bool cleanupVulkan();
	bool drawFrame(GLFWwindow* window);
	void markFramebuffersResized();
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

	void registerExtensionFunctions(VkInstance instance);

	bool pickPhysicalDevice();
	bool deviceIsSuitable(VkPhysicalDevice device);
	QueueFamiliesIndexStore findSuitableQueueFamilies(VkPhysicalDevice device);
	bool supportsDeviceExtensions(VkPhysicalDevice device);
	bool supportsDeviceFeatures(VkPhysicalDevice device);

	bool createLogicalDevice();
	bool createSurface(GLFWwindow* window);

	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window);
	bool createSwapChain(GLFWwindow* window);

	bool recreateSwapChain(GLFWwindow* window);
	void cleanupSwapChain();

	bool createImageViews();

	bool createCommandPool();
	bool createCommandBuffers();
	bool recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
	bool recordCommandBufferDynamicRendering(VkCommandBuffer commandBuffer, uint32_t imageIndex);

	void transitionImageLayout(VkCommandBuffer commandBuffer, uint32_t imageIndex, VkImageLayout oldLayout, VkImageLayout newLayout,
		VkAccessFlags2 srcAccessMask, VkAccessFlags2 dstAccessMask, VkPipelineStageFlags2 srcStageMask,
		VkPipelineStageFlags2 dstStageMask);

	bool createSyncObjects();

	VkInstance mInstance;
	VkDebugUtilsMessengerEXT mDebugMessanger;
	VkPhysicalDevice mPhysicalDevice = VK_NULL_HANDLE;
	VkDevice mLogicalDevice;

	VkQueue mGraphicsQueue;
	VkQueue mPresentQueue;

	VkSurfaceKHR mSurface;
	VkSwapchainKHR mSwapChain;
	std::vector<VkImage> mSwapChainImages;
	std::vector<VkImageView> mSwapChainImageViews;
	VkFormat mSwapChainImageFormat;
	VkExtent2D mSwapChainImageExtent;

	GraphicsPipeline mGraphicsPipeline;
	VertexBufferData mVertexBufferData;
	UniformBufferData mUniformBufferData;

	VkCommandPool mCommandPool;
	std::vector<VkCommandBuffer> mCommandBuffers;

	std::vector<VkSemaphore> mImageAvailableSemaphores;
	std::vector<VkSemaphore> mRenderFinishedSemaphores;
	std::vector<VkFence> mWhileRenderingFences;

	const int MAX_FRAMES_BEING_PROCESSED = 2;
	uint32_t mCurrentFrame = 0;

	bool mFramebuffersResized = false;

	void (*fpCmdBeginRenderingKHR)(VkCommandBuffer, const VkRenderingInfo*);
	void (*fpCmdEndRenderingKHR)(VkCommandBuffer);
	void(*fpCmdPipelineBarrier2)(VkCommandBuffer, const VkDependencyInfo*);

	const std::vector<const char*> mValidationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};

	std::vector<const char*> mRequiredDeviceExtension = { 
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
		VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
		VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME,
		VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME
	};
};

static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
	VulkanManager* vulkanManagerRef = reinterpret_cast<VulkanManager*>(glfwGetWindowUserPointer(window));
	vulkanManagerRef->markFramebuffersResized();
}