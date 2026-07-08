#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include <iostream>
#include <vector>
#include <set>
#include <algorithm>
#include <string>

#include "Pipelines/GraphicsPipeline.hpp"
#include "Pipelines/ComputePipeline.hpp"
#include "Mesh/Mesh3D.hpp"
#include "Mesh/BufferDrawer.hpp"
#include "UniformDescriptorHandlers/UniformDescriptorManager.hpp"
#include "UniformDescriptorHandlers/UniformDescriptors/UniformBufferDescriptor.hpp"
#include "UniformDescriptorHandlers/UniformDescriptors/UniformImageDescriptor.hpp"
#include "GameObject.hpp"
#include "RenderGraph.hpp"

#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else 
	const bool enableValidationLayers = true;
#endif

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData);

struct QueueFamiliesIndexStore {
	int graphicsFamalyIndex = -1;
	int computeFamalyIndex = -1;
	int presentFamalyIndex = -1;

	bool containsAllFamilies() const;
	std::set<uint32_t> getVectorOfIndecies();
};

enum Scene
{
	PARTICLES,
	MODEL
};

struct SceneData 
{
	const char* name = "";
	std::vector<DrawableData> sceneGameObjects = {};
	Scene sceneType = {};
	bool selected = false;

	//Explicit padding for compiler warning
	uint8_t padding[3] = {};
};

struct SwapChainSupportDetails
{
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
	VkSurfaceCapabilitiesKHR capabilities;

	//Explicit padding for x64 since struct size is not divisable by 8 in x64
	#if defined(_WIN64) || defined(__x86_64__)
		uint8_t padding[4];
	#endif
};

class VulkanManager {
public:
	bool initVulkan(GLFWwindow* window);
	bool cleanupVulkan();
	bool drawFrame(GLFWwindow* window, float dt, bool* needToReloadGUI_Flag = nullptr);
	void markFramebuffersResized();
	void updateGUI();
	void renderGUI(VkCommandBuffer commandBuffer);
	void renderGUI_DynamicRender(VkCommandBuffer commandBuffer, uint32_t imageIndex);

	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) const;

	//Inline one liners
	VkInstance getInstance() const;
	VkDevice getLogicalDevice() const;
	VkPhysicalDevice getPhysicalDevice() const;
	const VkFormat* getSwapChainImageFormatRef() const;
	VkFormat getDepthFormat() const;
	VkSampleCountFlagBits getMSAA_Samples() const;
	VkQueue getGraphicsQueue() const;

	//Explicit class defenitions to avoid compiler warnings
	VulkanManager() = default;
	VulkanManager(const VulkanManager&) = default;
	VulkanManager& operator=(VulkanManager&& other) = delete;
	VulkanManager& operator=(const VulkanManager& other) = delete;
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
	QueueFamiliesIndexStore findSuitableQueueFamilies(VkPhysicalDevice device) const;
	bool supportsDeviceExtensions(VkPhysicalDevice device);
	bool supportsDeviceFeatures(VkPhysicalDevice device);

	VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const;
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

	bool createSwapChainImageViews();
	bool createDepthResources();
	bool createMSAA_ColorResources();

	bool createCommandPool();
	bool createCommandBuffers();

	void handleInjectPipelineMemoryBarriers(VkCommandBuffer commandBuffer, Pipeline* sourcePipeline);

	bool createSyncObjects();

	void handlePipelineChanges(bool* needToReloadGUI_Flag);

	VkSampleCountFlagBits getMaxUsableSampleCount() const;

	ModelViewProjectionUniformObject mMVP_UniformObject{};

	UniformDescriptorManager mUniformDescriptorManager{};

	Mesh3D mHouseMesh{};

	VkDebugUtilsMessengerEXT mDebugMessanger{};

	//Main depth attachment data
	VkImage mDepthImage = VK_NULL_HANDLE;
	VkDeviceMemory mDepthMemory = VK_NULL_HANDLE;
	VkImageView mDepthImageView = VK_NULL_HANDLE;

	//Main color attachment for MSAA
	VkImage mMSAA_ColorImage = VK_NULL_HANDLE;
	VkDeviceMemory mMSAA_ColorhMemory = VK_NULL_HANDLE;
	VkImageView mMSAA_ColorImageView = VK_NULL_HANDLE;

	VkCommandPool mCommandPool = VK_NULL_HANDLE;

	VkSurfaceKHR mSurface = VK_NULL_HANDLE;
	VkSwapchainKHR mSwapChain = VK_NULL_HANDLE;

	//Main texture image data
	VkImage mTextureImage = VK_NULL_HANDLE;
	VkImageView mTextureImageView = VK_NULL_HANDLE;
	VkDeviceMemory mTextureMemory = VK_NULL_HANDLE;
	VkSampler mTextureSampler = VK_NULL_HANDLE;

	BufferDrawer mParticleDrawer{};

	RenderGraph mActiveRenderGraph{};

	SceneData mSelectedScene{};

	GameObject mHouseGameObject{};

	//Scene data for managing which scenes to render
	std::vector<SceneData> mScenes{};

	std::vector<GraphicsPipeline> mGraphicsPipelineStorageList{};
	std::vector<ComputePipeline> mComputePipelineStorageList{};

	//Rendering semaphores and fences
	std::vector<VkSemaphore> mImageAvailableSemaphores{};
	std::vector<VkSemaphore> mRenderFinishedSemaphores{};
	std::vector<VkFence> mWhileRenderingFences{};

	std::vector<VkImage> mSwapChainImages{};
	std::vector<VkImageView> mSwapChainImageViews{};

	std::vector<VkCommandBuffer> mCommandBuffers{};

	std::vector<const char*> mRequiredDeviceExtension = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
		VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
		VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME,
		VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME
	};

	const std::vector<const char*> mValidationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};

	VkInstance mInstance = VK_NULL_HANDLE;
	VkPhysicalDevice mPhysicalDevice = VK_NULL_HANDLE;
	VkDevice mLogicalDevice = VK_NULL_HANDLE;

	//Queue data
	VkQueue mGraphicsQueue = VK_NULL_HANDLE;
	VkQueue mComputeQueue = VK_NULL_HANDLE;
	VkQueue mPresentQueue = VK_NULL_HANDLE;

	VkExtent2D mSwapChainImageExtent{};

	//Function pointers for additional feature functions
	void(__stdcall* fpCmdBeginRenderingKHR)(VkCommandBuffer, const VkRenderingInfo*) {};
	void(__stdcall* fpCmdEndRenderingKHR)(VkCommandBuffer) {};
	void(__stdcall* fpCmdPipelineBarrier2)(VkCommandBuffer, const VkDependencyInfo*) {};

	VkFormat mSwapChainImageFormat{};
	VkFormat mDepthFormat{};

	const uint32_t MAX_FRAMES_IN_FLIGHT = 2;
	const uint32_t PARTICLE_COUNT = 256000;

	uint32_t mCurrentFrame = 0;
	uint32_t mCurrScene = 0;

	DeltaTimeUniformObject mDtUniformObject{};

	VkSampleCountFlagBits mMSAA_Samples = VK_SAMPLE_COUNT_1_BIT;

	bool mRenderingParticles = true;
	bool mFramebuffersResized = false;
	bool mSwapScenesTriggered = false;
	bool mSwitchingRenderMethod = false;
	bool mUsingDynamicRenderingForGUI = true; //For ImGui display, to not imediately switch with the one for logic while still rendering
	bool mUsingDynamicRendering = true;

	//Extra padding for x64 since x64 does not align to 16 but x86 does
	#if defined(_WIN64) || defined(__x86_64__)
		#if (defined(_DEBUG))
		uint8_t padding[2]{};
		#else
			uint8_t padding[2]{}; //Contributed by RenderGraph vector member
		#endif
	#else
		#if (defined(_DEBUG))
			uint8_t padding[2]{};
		#else
			uint8_t padding[14]{}; //Contributed by RenderGraph and UniformDescriptorManager vector member
		#endif
	#endif
};

void framebufferResizeCallback(GLFWwindow* window, int width, int height) noexcept;