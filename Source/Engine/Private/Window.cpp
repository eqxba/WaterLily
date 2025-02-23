#include "Engine/Window.hpp"

#include <GLFW/glfw3.h>

#include "Engine/EventSystem.hpp"

namespace WindowDetails
{
    static GLFWwindow* CreateGlfwWindow(Extent2D extent, const std::string_view title, const WindowMode mode)
    {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        GLFWmonitor* monitor = nullptr;

        switch (mode)
        {
        case WindowMode::eWindowed:
            break;
        case WindowMode::eFullscreenWindowed:
            glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
            glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
            break;
        case WindowMode::eFullscreen:
            monitor = glfwGetPrimaryMonitor();
            break;
        }

        if (monitor)
        {
            const GLFWvidmode* videoMode = glfwGetVideoMode(monitor);
            extent.width = videoMode->width;
            extent.height = videoMode->height;
        }

        return glfwCreateWindow(extent.width, extent.height, title.data(), monitor, nullptr);
    }

    static KeyMods GetKeyMods(const int mods)
    {
        auto keyMods = KeyMods::eNone;

        if (mods & GLFW_MOD_SHIFT)
        {
            keyMods = keyMods | KeyMods::eShift;
        }
        if (mods & GLFW_MOD_CONTROL)
        {
            keyMods = keyMods | KeyMods::eCtrl;
        }
        if (mods & GLFW_MOD_ALT)
        {
            keyMods = keyMods | KeyMods::eAlt;
        }
        if (mods & GLFW_MOD_SUPER)
        {
            keyMods = keyMods | KeyMods::eSuper;
        }

        return keyMods;
    }
}

Window::Window(const WindowDescription& description, EventSystem& aEventSystem)
    : eventSystem{ aEventSystem }
    , extent{ description.extent }
    , title{ description.title }
    , mode{ description.mode }
    , cursorMode{ description.cursorMode }
{
    glfwInit();

    Init();
}

Window::~Window()
{
    Cleanup();
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

void Window::SetMode(const WindowMode aMode)
{
    if (mode == aMode)
    {
        return;
    }

    mode = aMode;

    eventSystem.Fire<ES::BeforeWindowRecreated>();

    Cleanup();

    Init();

    eventSystem.Fire<ES::WindowRecreated>({ this });
}

void Window::SetCursorMode(const CursorMode aCursorMode, const bool force /* = false */)
{
    if (cursorMode == aCursorMode && !force)
    {
        return;
    }

    eventSystem.Fire<ES::BeforeCursorModeUpdated>({ aCursorMode });

    cursorMode = aCursorMode;

    if (cursorMode == CursorMode::eDisabled)
    {
        glfwSetInputMode(glfwWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        glfwSetInputMode(glfwWindow, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }
    else if (cursorMode == CursorMode::eEnabled)
    {
        glfwSetInputMode(glfwWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        glfwSetInputMode(glfwWindow, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);

        CenterCursor();
    }
}

void Window::FramebufferSizeCallback(GLFWwindow* glfwWindow, const int32_t width, const int32_t height)
{
    EventSystem& eventSystem = static_cast<Window*>(glfwGetWindowUserPointer(glfwWindow))->eventSystem;    
    eventSystem.Fire<ES::WindowResized>({ { width, height } });
}

void Window::WindowSizeCallback(GLFWwindow* glfwWindow, const int32_t width, const int32_t height)
{
    const auto window = static_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));

    window->extent = { width, height };

    if (window->mode == WindowMode::eWindowed)
    {
        window->extentInWindowedMode = window->extent;
    }
}

void Window::KeyCallback(GLFWwindow* glfwWindow, int key, int scancode, int action, int mods)
{
    EventSystem& eventSystem = static_cast<Window*>(glfwGetWindowUserPointer(glfwWindow))->eventSystem;
    eventSystem.Fire<ES::KeyInput>({ .key = static_cast<Key>(key),
        .action = static_cast<KeyAction>(action), .mods = WindowDetails::GetKeyMods(mods)});
}

void Window::MouseCallback(GLFWwindow* glfwWindow, const double xPos, const double yPos)
{
    EventSystem& eventSystem = static_cast<Window*>(glfwGetWindowUserPointer(glfwWindow))->eventSystem;
    eventSystem.Fire<ES::MouseMoved>({ .newPosition = { static_cast<float>(xPos), static_cast<float>(yPos) } });
}

void Window::MouseButtonCallback(GLFWwindow* glfwWindow, const int button, const int action, const int mods)
{
    EventSystem& eventSystem = static_cast<Window*>(glfwGetWindowUserPointer(glfwWindow))->eventSystem;
    eventSystem.Fire<ES::MouseInput>({ .button = static_cast<MouseButton>(button),
        .action = static_cast<MouseButtonAction>(action), .mods = WindowDetails::GetKeyMods(mods) });
}

void Window::ScrollCallback(GLFWwindow* glfwWindow, const double xOffset, const double yOffset)
{
    EventSystem& eventSystem = static_cast<Window*>(glfwGetWindowUserPointer(glfwWindow))->eventSystem;
    eventSystem.Fire<ES::MouseWheelScrolled>({ .offset = { static_cast<float>(xOffset), static_cast<float>(yOffset) } });
}

void Window::Init()
{
    if (extentInWindowedMode && mode == WindowMode::eWindowed)
    {
        extent = extentInWindowedMode.value();
    }

    glfwWindow = WindowDetails::CreateGlfwWindow(extent, title, mode);

    glfwGetWindowSize(glfwWindow, &extent.width, &extent.height);
    glfwGetFramebufferSize(glfwWindow, &extentInPixels.width, &extentInPixels.height);

    if (mode == WindowMode::eWindowed)
    {
        extentInWindowedMode = extent;
    }

    SetCursorMode(cursorMode, true);
    RegisterCallbacks();
}

void Window::Cleanup()
{
    glfwDestroyWindow(glfwWindow);
}

void Window::RegisterCallbacks()
{
    glfwSetWindowUserPointer(glfwWindow, this);

    glfwSetFramebufferSizeCallback(glfwWindow, FramebufferSizeCallback);
    glfwSetWindowSizeCallback(glfwWindow, WindowSizeCallback);
    glfwSetKeyCallback(glfwWindow, KeyCallback);
    glfwSetCursorPosCallback(glfwWindow, MouseCallback);
    glfwSetMouseButtonCallback(glfwWindow, MouseButtonCallback);
    glfwSetScrollCallback(glfwWindow, ScrollCallback);
}

void Window::CenterCursor()
{
    glfwSetCursorPos(glfwWindow, extent.width / 2.0, extent.height / 2.0);
}

void Window::OnResize(const ES::WindowResized& event)
{
    extentInPixels = event.newExtent;
}
