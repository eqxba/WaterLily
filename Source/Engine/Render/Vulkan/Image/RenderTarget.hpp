#pragma once

#include <volk.h>

#include "Engine/Render/Vulkan/Image/ImageView.hpp"

class VulkanContext;

struct RenderTarget
{
public:
    RenderTarget() = default;
    RenderTarget(ImageDescription description, VkImageAspectFlags aspectMask, const VulkanContext& vulkanContext);
    RenderTarget(VkImage image, ImageDescription description, VkImageAspectFlags aspectMask, const VulkanContext& vulkanContext, bool isSwapchainImage = false);
    
    bool IsValid() const
    {
        return image.IsValid() && std::ranges::all_of(views, &ImageView::IsValid);
    }
    
    operator const Image&() const
    {
        return image;
    }
    
    Image image;
    std::vector<ImageView> views;
    
    ImageView textureView; // For sampling all mips of the RenderTarget, exists only when mipLevelsCount > 1
    
private:
    void CreateViews(VkImageAspectFlags aspectMask, const VulkanContext& vulkanContext);
};
