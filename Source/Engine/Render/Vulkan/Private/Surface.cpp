#include "Engine/Render/Vulkan/Surface.hpp"

#include "Engine/Window.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Instance.hpp"

#include <GLFW/glfw3.h>

Surface::Surface(const Window& window)
{
	const VkResult result
		= glfwCreateWindowSurface(VulkanContext::instance->instance, window.glfwWindow, nullptr, &surface);
	Assert(result == VK_SUCCESS);
}

Surface::~Surface()
{
	vkDestroySurfaceKHR(VulkanContext::instance->instance, surface, nullptr);
}