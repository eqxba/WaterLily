#pragma once

#include "Utils/DataStructures.hpp"

namespace ES
{
	struct WindowResized;
}

struct GLFWwindow;
class EventSystem;

enum class WindowMode
{
	eWindowed,
	eFullscreenWindowed,
	eFullscreen,
};

struct WindowDescription
{
	Extent2D extent{ 960, 540 };
	std::string_view title;
	WindowMode mode = WindowMode::eWindowed;
	CursorMode cursorMode = CursorMode::eDisabled;
};

class Window
{
public:
	Window(const WindowDescription& description, EventSystem& eventSystem);
	~Window();

	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;

	Window(Window&&) = delete;
	Window& operator=(Window&&) = delete;

	bool ShouldClose() const;
	void PollEvents() const;

	Extent2D GetExtentInPixels() const
	{
		return extentInPixels;
	}

	GLFWwindow* GetGlfwWindow() const
	{
		return glfwWindow;
	}

	WindowMode GetMode() const
	{
		return mode;
	}

	void SetMode(WindowMode mode);

	CursorMode GetCursorMode() const
	{
		return cursorMode;
	}

	void SetCursorMode(CursorMode cursorMode, bool force = false);

private:
	static void FramebufferSizeCallback(GLFWwindow* glfwWindow, int32_t width, int32_t height);
	static void WindowSizeCallback(GLFWwindow* glfwWindow, int32_t width, int32_t height);
	static void KeyCallback(GLFWwindow* glfwWindow, int key, int scancode, int action, int mods);
	static void MouseCallback(GLFWwindow* glfwWindow, double xPos, double yPos);
	static void ScrollCallback(GLFWwindow* glfwWindow, double xOffset, double yOffset);

	void Init();
	void Cleanup();

	void RegisterCallbacks();

	void CenterCursor();

	void OnResize(const ES::WindowResized& event);

	EventSystem& eventSystem;

	GLFWwindow* glfwWindow = nullptr;

	Extent2D extent{};
	Extent2D extentInPixels{};
	std::optional<Extent2D> extentInWindowedMode{};
	std::string_view title{};
	WindowMode mode = WindowMode::eWindowed;
	CursorMode cursorMode = CursorMode::eDisabled;
};