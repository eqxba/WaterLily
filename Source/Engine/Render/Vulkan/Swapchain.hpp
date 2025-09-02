#pragma once

#include "Engine/Window.hpp"
#include "Engine/Render/Vulkan/Image/RenderTarget.hpp"

#include <volk.h>

class VulkanContext;

struct SwapchainSupportDetails
{
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class Swapchain
{
public:
    Swapchain(const VkExtent2D& requiredExtentInPixels, const VulkanContext& aVulkanContext);
    ~Swapchain();

    Swapchain(const Swapchain&) = delete;
    Swapchain& operator=(const Swapchain&) = delete;

    Swapchain(Swapchain&&) = delete;
    Swapchain& operator=(Swapchain&&) = delete;

    // Call in case of simple resize (not when window's been recreated!)
    void Recreate(const VkExtent2D& requiredExtentInPixels);

    VkSurfaceFormatKHR GetSurfaceFormat() const
    {
        return surfaceFormat;
    }

    VkExtent2D GetExtent() const
    {
        return extent;
    }
    
    VkRect2D GetRect() const
    {
        return { { 0, 0 }, extent };
    }

    const std::vector<RenderTarget>& GetRenderTargets() const
    {
        return renderTargets;
    }
    
    operator VkSwapchainKHR() const
    {
        return swapchain;
    }

private:
    void Create(const VkExtent2D& requiredExtentInPixels);
    void Cleanup();
    
    ImageDescription GetRenderTargetDescription() const;

    const VulkanContext& vulkanContext;
    
    SwapchainSupportDetails supportDetails;

    VkSwapchainKHR swapchain;

    VkSurfaceFormatKHR surfaceFormat; 
    VkExtent2D extent;

    std::vector<RenderTarget> renderTargets;
};
