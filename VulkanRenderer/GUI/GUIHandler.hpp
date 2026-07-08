#pragma once

#include <vulkan/vulkan.h>
#include <glfw/glfw3.h>
#include "VulkanRenderer/VulkanManager.hpp"

class GUIHandler {
public:
	void initImGui(GLFWwindow* window, VulkanManager* vulkanManager);
	void checkGUI_State(GLFWwindow* window, VulkanManager* vulkanManager);
	void cleanupGUI(VkDevice logicalDevice);

	bool* getReloadGUI_Flag();
private:
	VkDescriptorPool mImGuiDescriptorPool = VK_NULL_HANDLE;
	bool mNeedToReloadGUI = false;

	//Explicit padding for compiler warning
	uint8_t padding[7] = {};
};