#include "Engine/Render/Vulkan/Image/RenderTarget.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"

RenderTarget::RenderTarget(ImageDescription description,
    const VkImageAspectFlags aspectMask, const VulkanContext& vulkanContext)
    : image{ std::move(description), vulkanContext }
{
    CreateViews(aspectMask, vulkanContext);
}

RenderTarget::RenderTarget(const VkImage aImage, ImageDescription description, const VkImageAspectFlags aspectMask,
    const VulkanContext& vulkanContext, const bool isSwapchainImage /* = false */)
    : image{ aImage, std::move(description), vulkanContext, isSwapchainImage }
{
    CreateViews(aspectMask, vulkanContext);
}

void RenderTarget::CreateViews(const VkImageAspectFlags aspectMask, const VulkanContext& vulkanContext)
{
    std::ranges::transform(std::views::iota(static_cast<uint32_t>(0), image.GetDescription().mipLevelsCount),
        std::back_inserter(views), [&](const uint32_t mipLevel) { return ImageView(image, aspectMask, mipLevel, 1, vulkanContext); } );
    
    Assert(!views.empty());
    
    if (views.size() > 1)
    {
        textureView = ImageView(image, aspectMask, 0, image.GetDescription().mipLevelsCount, vulkanContext);
    }
}
