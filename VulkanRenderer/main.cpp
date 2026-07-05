#include "VulkanManager.h"
#include "WindowManager.h"
#include "GUI/GUIHandler.h"

#include <iostream>

int main() {
    VulkanManager vulkanManager;
    WindowManager windowManager;
    GUIHandler guiHandler;

    //Initializes window and vulkan
    windowManager.initWindow();
    vulkanManager.initVulkan(windowManager.getWindowRef());
    guiHandler.initImGui(windowManager.getWindowRef(), &vulkanManager);

    float currTime = 0, prevTime = 0, dt = 0;
    while (!windowManager.shouldCloseWindow()) {
        windowManager.pollWindowEvents();

        vulkanManager.drawFrame(windowManager.getWindowRef(), dt, guiHandler.getReloadGUI_Flag());

        guiHandler.checkGUI_State(windowManager.getWindowRef(), &vulkanManager);

        currTime = static_cast<float>(glfwGetTime());
        dt = (currTime - prevTime) * 1000.0f;
        prevTime = currTime;
    }

    guiHandler.cleanupGUI(vulkanManager.getLogicalDevice());
    vulkanManager.cleanupVulkan();
    windowManager.cleanup();

    return 0;
}