#pragma once

#include "Utils/DataStructures.hpp"

struct GLFWwindow;
class EventSystem;

class Window
{
public:
	Window(int width, int height, std::string title, EventSystem& aEventSystem);
	~Window();

	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;

	Window(Window&&) = delete;
	Window& operator=(Window&&) = delete;

	bool ShouldClose() const;
	void PollEvents() const;

	Extent2D GetExtentInPixels() const;

	GLFWwindow* GetGlfwWindow() const
	{
		return glfwWindow;
	}

private:
	static void ResizeCallback(GLFWwindow* glfwWindow, int32_t width, int32_t height);

	EventSystem& eventSystem;

	GLFWwindow* glfwWindow;
};