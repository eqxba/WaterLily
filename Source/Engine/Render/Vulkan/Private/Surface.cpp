#include "Engine/Render/Vulkan/Surface.hpp"

#include "Engine/Window.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"

#include <GLFW/glfw3.h>

Surface::Surface(const Window& window, const VulkanContext& aVulkanContext)
    : vulkanContext{aVulkanContext}
{
    const VkResult result = glfwCreateWindowSurface(vulkanContext.GetInstance().GetVkInstance(), 
        window.GetGlfwWindow(), nullptr, &surface);
    Assert(result == VK_SUCCESS);
}

Surface::~Surface()
{
    vkDestroySurfaceKHR(vulkanContext.GetInstance().GetVkInstance(), surface, nullptr);
}