#pragma once

#include "Engine/Scene/Scene.hpp"
#include "Engine/InputHelpers.hpp"

namespace ES // Event system
{
	struct WindowResized
	{
		Extent2D newExtent = {};
	};

	struct SceneOpened
	{
		Scene& scene;
	};

	struct KeyInput
	{
		Key key;
		KeyAction action;
	};

	struct MouseMoved
	{
		glm::vec2 newPosition;
	};

	struct MouseWheelScrolled
	{
		glm::vec2 offset;
	};
}
