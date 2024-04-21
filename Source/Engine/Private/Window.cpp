#include "Engine/Window.hpp"

#include <GLFW/glfw3.h>

#include "Engine/Engine.hpp"
#include "Engine/EventSystem.hpp"

Window::Window(int width, int height, std::string title, EventSystem& aEventSystem)
	: eventSystem{ aEventSystem }
{
	glfwInit();	
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	
	glfwWindow = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
	
	glfwSetWindowUserPointer(glfwWindow, this);

	glfwSetInputMode(glfwWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	glfwSetFramebufferSizeCallback(glfwWindow, ResizeCallback);
	glfwSetKeyCallback(glfwWindow, KeyCallback);
	glfwSetCursorPosCallback(glfwWindow, MouseCallback);
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
	EventSystem& eventSystem = static_cast<Window*>(glfwGetWindowUserPointer(glfwWindow))->eventSystem;	
	eventSystem.Fire<ES::WindowResized>({ .newWidth = width, .newHeight = height });
}

void Window::KeyCallback(GLFWwindow* glfwWindow, int key, int scancode, int action, int mods)
{
	EventSystem& eventSystem = static_cast<Window*>(glfwGetWindowUserPointer(glfwWindow))->eventSystem;
	eventSystem.Fire<ES::KeyInput>({ .key = static_cast<Key>(key), .action = static_cast<KeyAction>(action)});
}

void Window::MouseCallback(GLFWwindow* glfwWindow, double xPos, double yPos)
{
	EventSystem& eventSystem = static_cast<Window*>(glfwGetWindowUserPointer(glfwWindow))->eventSystem;
	eventSystem.Fire<ES::MouseMoved>({ .newPosition = { static_cast<float>(xPos), static_cast<float>(yPos) } });
}
