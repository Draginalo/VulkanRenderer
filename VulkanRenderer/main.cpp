#include "VulkanManager.hpp"
#include "WindowManager.hpp"
#include "GUI/GUIHandler.hpp"

#include <iostream>

int main() {
    VulkanManager vulkanManager;
    WindowManager windowManager;
    GUIHandler guiHandler;

    //Initializes window and vulkan
    windowManager.initWindow();
    vulkanManager.initVulkan(windowManager.getWindowRef());
    guiHandler.initImGui(windowManager.getWindowRef(), &vulkanManager);

    double currTime = 0, prevTime = 0, dt = 0;
    while (!windowManager.shouldCloseWindow()) {
        windowManager.pollWindowEvents();

        vulkanManager.drawFrame(windowManager.getWindowRef(), static_cast<float>(dt), guiHandler.getReloadGUI_Flag());

        guiHandler.checkGUI_State(windowManager.getWindowRef(), &vulkanManager);

        currTime = glfwGetTime();
        dt = (currTime - prevTime) * 1000.0f;
        prevTime = currTime;
    }

    guiHandler.cleanupGUI(vulkanManager.getLogicalDevice());
    vulkanManager.cleanupVulkan();
    windowManager.cleanup();

    return 0;
}