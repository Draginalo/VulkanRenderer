#include "VulkanManager.h"
#include "WindowManager.h"

#include <iostream>

int main() {
    VulkanManager vulkanManager;
    WindowManager windowManager;

    //Initializes window and vulkan
    windowManager.initWindow();
    vulkanManager.initVulkan(windowManager.getWindowRef());
    vulkanManager.initImGui(windowManager.getWindowRef());

    float currTime = 0, prevTime = 0, dt = 0;
    while (!windowManager.shouldCloseWindow()) {
        windowManager.pollWindowEvents();
        vulkanManager.drawFrame(windowManager.getWindowRef(), dt);

        currTime = glfwGetTime();
        dt = (currTime - prevTime) * 1000.0f;
        prevTime = currTime;

        //vulkanManager.handleGUI();
    }

    vulkanManager.cleanupGUI();
    vulkanManager.cleanupVulkan();
    windowManager.cleanup();

    return 0;
}