#pragma once

#include "Engine/Scene/Scene.hpp"
#include "Engine/InputHelpers.hpp"

namespace ES // Event system
{
	struct WindowResized
	{
		int newWidth = 0;
		int newHeight = 0;
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
}
