#include "Engine/Window.hpp"

#include <GLFW/glfw3.h>

// TODO: Temp
#include "Engine/Engine.hpp"

Window::Window(int width, int height, std::string title)
{
	glfwInit();
	
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	
	glfwWindow = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
	glfwSetFramebufferSizeCallback(glfwWindow, ResizeCallback);
}

Window::~Window()
{
	glfwDestroyWindow(glfwWindow);
	glfwTerminate();
}

bool Window::ShouldClose() const
{
	return glfwWindowShouldClose(glfwWindow);
}

void Window::PollEvents() const
{
	glfwPollEvents();
}

Extent2D Window::GetExtentInPixels() const
{
	Extent2D extent;
	glfwGetFramebufferSize(glfwWindow, &extent.width, &extent.height);
	return extent;
}

void Window::ResizeCallback(GLFWwindow* glfwWindow, int32_t width, int32_t height)
{
	// TODO: Implement EventSystem
	Engine::OnResize({ width, height });
}