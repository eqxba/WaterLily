#pragma once

#include <volk.h>

class Window;

class Surface
{
public:
	Surface(const Window& window);
	~Surface();

	VkSurfaceKHR surface;
};