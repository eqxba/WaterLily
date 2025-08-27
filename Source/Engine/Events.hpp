#pragma once

#include "Engine/Window.hpp"
#include "Engine/Scene/Scene.hpp"
#include "Engine/InputUtils.hpp"

namespace ES // Event system
{
    struct WindowResized
    {
        Extent2D newExtent{};
    };

    struct BeforeWindowRecreated {};

    struct WindowRecreated
    {
        const Window* window;
    };

    struct BeforeSwapchainRecreated {};
    struct SwapchainRecreated {};

    struct TryReloadShaders {};

    struct SceneOpened
    {
        Scene& scene;
    };

    struct SceneClosed {};

    struct KeyInput
    {
        Key key{};
        KeyAction action{};
        KeyMods mods{};
    };

    struct MouseMoved
    {
        glm::vec2 newPosition{};
    };

    struct MouseInput
    {
        MouseButton button{};
        MouseButtonAction action{};
        KeyMods mods{};
    };

    struct MouseWheelScrolled
    {
        glm::vec2 offset{};
    };

    struct BeforeInputModeUpdated
    {
        InputMode newInputMode{};
    };
}
