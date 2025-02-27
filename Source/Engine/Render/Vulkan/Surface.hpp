#pragma once

#include <volk.h>

class VulkanContext;
class Window;

class Surface
{
public:
    Surface(const Window& window, const VulkanContext& aVulkanContext);
    ~Surface();

    Surface(const Surface&) = delete;
    Surface& operator=(const Surface&) = delete;

    Surface(Surface&&) = delete;
    Surface& operator=(Surface&&) = delete;

    VkSurfaceKHR GetVkSurfaceKHR() const
    {
        return surface;
    }

private:
    const VulkanContext& vulkanContext;

    VkSurfaceKHR surface;
};