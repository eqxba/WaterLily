#include "Engine/Window.hpp"

#include <GLFW/glfw3.h>

Window::Window(int width, int height, std::string title)
{
	glfwInit();
	
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	
	glfwWindow = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
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
