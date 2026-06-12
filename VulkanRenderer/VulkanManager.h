#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include "vulkan/vulkan.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include <iostream>
#include <vector>
#include <set>
#include <algorithm>
#include <string>

#include "PipelineData.h"
#include "Mesh/Mesh3D.h"
#include "UniformBufferData.h"
#include "Helpers/TextureImageHelpers.h"

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
	int computeFamalyIndex = -1;
	int presentFamalyIndex = -1;

	bool containsAllFamilies()
	{
		return graphicsFamalyIndex != -1 && computeFamalyIndex != -1 && presentFamalyIndex != -1;
	}

	std::set<uint32_t> getVectorOfIndecies()
	{
		return std::set<uint32_t> {
			static_cast<uint32_t>(graphicsFamalyIndex),
			static_cast<uint32_t>(computeFamalyIndex),
			static_cast<uint32_t>(presentFamalyIndex)
		};
	}
};

enum Scene
{
	PARTICLES,
	MODEL
};

struct SceneData {

	Scene sceneType;
	std::string name;
	bool selected;
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
	bool drawFrame(GLFWwindow* window, float dt, bool* needToReloadGUI_Flag = nullptr);
	void markFramebuffersResized();
	void updateGUI();
	void renderGUI(VkCommandBuffer commandBuffer, int imageIndex);
	void renderGUI_DynamicRender(VkCommandBuffer commandBuffer, int imageIndex);

	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

	inline const VkInstance getInstance() { return mInstance; }
	inline const VkDevice getLogicalDevice() { return mLogicalDevice; }
	inline const VkPhysicalDevice getPhysicalDevice() { return mPhysicalDevice; }
	inline const VkFormat* getSwapChainImageFormatRef() { return &mSwapChainImageFormat; }
	inline const VkFormat getDepthFormat() { return mDepthFormat; }
	inline const VkSampleCountFlagBits getMSAA_Samples() { return mMSAA_Samples; }
	inline PipelineData* getPipelineData() { return &mPipelineData; }
	inline const VkQueue getGraphicsQueue() { return mGraphicsQueue; }
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

	VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	VkFormat findDepthFormat();
	bool hasStencilComponent(VkFormat format);

	bool createLogicalDevice();
	bool createSurface(GLFWwindow* window);

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window);
	bool createSwapChain(GLFWwindow* window);

	bool recreateSwapChain(GLFWwindow* window);
	void cleanupSwapChain();

	bool createImageViews();
	bool createDepthResources();
	bool createMSAA_ColorResources();

	bool createCommandPool();
	bool createCommandBuffers();
	bool renderScene(VkCommandBuffer commandBuffer, uint32_t imageIndex);
	bool renderScene_DynamicRendering(VkCommandBuffer commandBuffer, uint32_t imageIndex);

	bool recordComputeCommandBuffer(VkCommandBuffer commandBuffer);

	bool createSyncObjects();

	void handlePipelineChanges(GLFWwindow* window, bool* needToReloadGUI_Flag);

	VkSampleCountFlagBits getMaxUsableSampleCount();

	VkInstance mInstance;
	VkPhysicalDevice mPhysicalDevice = VK_NULL_HANDLE;
	VkDevice mLogicalDevice;

	//Queue data
	VkQueue mGraphicsQueue;
	VkQueue mComputeQueue;
	VkQueue mPresentQueue;

	VkSurfaceKHR mSurface;
	VkSwapchainKHR mSwapChain;
	std::vector<VkImage> mSwapChainImages;
	std::vector<VkImageView> mSwapChainImageViews;
	VkFormat mSwapChainImageFormat;
	VkExtent2D mSwapChainImageExtent;
	VkFormat mDepthFormat;

	PipelineData mPipelineData;
	Mesh3D mVertexBufferData;
	UniformBufferData mUniformBufferData;

	VkCommandPool mCommandPool;
	std::vector<VkCommandBuffer> mGraphicsCommandBuffers;
	std::vector<VkCommandBuffer> mComputeCommandBuffers;

	//Graphics semaphores and fences
	std::vector<VkSemaphore> mImageAvailableSemaphores;
	std::vector<VkSemaphore> mRenderFinishedSemaphores;
	std::vector<VkFence> mWhileRenderingFences;

	//Compute semaphores and fences
	std::vector<VkSemaphore> mComputeFinishedSemaphores;
	std::vector<VkFence> mWhileComputingFences;

	const int MAX_FRAMES_BEING_PROCESSED = 2;
	const int PARTICLE_COUNT = 256000;

	uint32_t mCurrentFrame = 0;

	bool mRenderingParticles = false;
	bool mFramebuffersResized = false;
	bool mRemakePipelineTriggered = false;
	bool mSwitchingRenderMethod = false;
	bool mUsingDynamicRenderingForGUI = false; //For ImGui display, to not imediately switch with the one for logic while still rendering

	//Scene data for managing which scenes to render
	std::vector<SceneData> mScenes = { { MODEL, "Render Model", true}, { PARTICLES, "Render Particles", false}};
	SceneData mSelectedScene;

	//Function pointers for additional feature functions
	void (*fpCmdBeginRenderingKHR)(VkCommandBuffer, const VkRenderingInfo*);
	void (*fpCmdEndRenderingKHR)(VkCommandBuffer);
	void(*fpCmdPipelineBarrier2)(VkCommandBuffer, const VkDependencyInfo*);

	//Main texture image data
	VkImage mTextureImage;
	VkDeviceMemory mTextureMemory;
	VkImageView mTextureImageView;
	VkSampler mTextureSampler;

	//Main depth attachment data
	VkImage mDepthImage;
	VkDeviceMemory mDepthMemory;
	VkImageView mDepthImageView;

	//Main color attachment for MSAA
	VkImage mMSAA_ColorImage;
	VkDeviceMemory mMSAA_ColorhMemory;
	VkImageView mMSAA_ColorImageView;

	VkSampleCountFlagBits mMSAA_Samples = VK_SAMPLE_COUNT_1_BIT;

	VkDebugUtilsMessengerEXT mDebugMessanger;

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