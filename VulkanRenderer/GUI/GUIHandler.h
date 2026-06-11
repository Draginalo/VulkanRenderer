#pragma once

#include <vulkan/vulkan.h>
#include <glfw/glfw3.h>
#include "../VulkanManager.h"

class GUIHandler {
public:
	void initImGui(GLFWwindow* window, VulkanManager* vulkanManager);
	void checkGUI_State(GLFWwindow* window, VulkanManager* vulkanManager);
	void cleanupGUI(VkDevice logicalDevice);

	inline bool* getReloadGUI_Flag() { return &mNeedToReloadGUI; }
private:
	VkDescriptorPool mImGuiDescriptorPool;
	bool mNeedToReloadGUI = false;
};