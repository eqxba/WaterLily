#pragma once

#include "Engine/Window.hpp"
#include "Engine/Scene/Scene.hpp"
#include "Engine/InputHelpers.hpp"

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

	struct SceneOpened
	{
		Scene& scene;
	};

	struct KeyInput
	{
		Key key{};
		KeyAction action{};
	};

	struct MouseMoved
	{
		glm::vec2 newPosition{};
	};

	struct MouseWheelScrolled
	{
		glm::vec2 offset{};
	};

	struct BeforeCursorModeUpdated
	{
		CursorMode newCursorMode{};
	};
}
