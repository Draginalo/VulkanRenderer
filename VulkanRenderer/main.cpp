#include "VulkanManager.h"
#include "WindowManager.h"

#include <iostream>

int main() {
    VulkanManager vulkanManager;
    WindowManager windowManager;

    //Initializes window and vulkan
    windowManager.initWindow();
    vulkanManager.initVulkan();

    while (!windowManager.shouldCloseWindow()) {
        windowManager.pollWindowEvents();
    }

    vulkanManager.cleanupVulkan();
    windowManager.cleanup();

    return 0;
}