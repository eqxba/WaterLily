#pragma once

struct GLFWwindow;

class Window
{
public:
	Window(int width, int height, std::string title);
	~Window();

	bool ShouldClose() const;
	void PollEvents() const;

	GLFWwindow* glfwWindow;
};