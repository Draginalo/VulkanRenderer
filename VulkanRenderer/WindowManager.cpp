#include "WindowManager.h"

bool WindowManager::initWindow()
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Renderer", nullptr, nullptr);

	std::cout << "Window succesfully created" << std::endl;

	return false;
}

bool WindowManager::cleanup()
{
	glfwDestroyWindow(window);

	glfwTerminate();

	std::cout << "Window and GLFW cleaned up" << std::endl;

	return false;
}

void WindowManager::pollWindowEvents()
{
	glfwPollEvents();
}

bool WindowManager::shouldCloseWindow()
{
	return glfwWindowShouldClose(window);
}
