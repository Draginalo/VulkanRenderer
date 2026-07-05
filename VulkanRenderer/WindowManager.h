#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>

constexpr uint32_t WIDTH = 800;
constexpr uint32_t HEIGHT = 600;

class WindowManager {
public:
	bool initWindow();
	bool cleanup();

	void pollWindowEvents();

	bool shouldCloseWindow();

	GLFWwindow* getWindowRef();
private:
	GLFWwindow* mpWindow;
};