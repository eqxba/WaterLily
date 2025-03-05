#pragma once

#include <volk.h>

#include "Engine/Render/Vulkan/Image/ImageView.hpp"

class VulkanContext;

struct RenderTarget
{
    RenderTarget() = default;
    RenderTarget(ImageDescription description, VkImageAspectFlags aspectFlags, const VulkanContext& vulkanContext);
    RenderTarget(VkImage image, ImageDescription description, VkImageAspectFlags aspectFlags,
        const VulkanContext& vulkanContext, bool isSwapchainImage = false);
    
    bool IsValid() const
    {
        return image.IsValid() && view.IsValid();
    }
    
    operator const Image&() const
    {
        return image;
    }
    
    Image image;
    ImageView view;
};
