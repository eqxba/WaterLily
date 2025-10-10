#include "Engine/Render/Vulkan/Image/ImageView.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"

namespace ImageViewDetails
{
    static VkImageView CreateImageView(VkDevice device, VkImage image, const ImageDescription& description,
        const VkImageAspectFlags aspectMask, const uint32_t baseMipLevel, const uint32_t levelCount)
    {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = image;
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = description.format;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = aspectMask;
        createInfo.subresourceRange.baseMipLevel = baseMipLevel;
        createInfo.subresourceRange.levelCount = levelCount;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        VkImageView imageView;
        const VkResult result = vkCreateImageView(device, &createInfo, nullptr, &imageView);
        Assert(result == VK_SUCCESS);

        return imageView;
    }
}

ImageView::ImageView(VkImage image, const ImageDescription& description, VkImageAspectFlags aAspectMask,
    const uint32_t baseMipLevel, const uint32_t levelCount, const VulkanContext& aVulkanContext)
    : vulkanContext{ &aVulkanContext }
    , aspectMask{ aAspectMask }
{
    imageView = ImageViewDetails::CreateImageView(vulkanContext->GetDevice(), image, description, aspectMask, baseMipLevel, levelCount);
}

ImageView::ImageView(const Image& image, VkImageAspectFlags aspectMask, const uint32_t baseMipLevel,
    const uint32_t levelCount, const VulkanContext& aVulkanContext)
    : ImageView(image, image.GetDescription(), aspectMask, baseMipLevel, levelCount, aVulkanContext)
{}

ImageView::~ImageView()
{
    if (IsValid())
    {
        vkDestroyImageView(vulkanContext->GetDevice(), imageView, nullptr);
        imageView = VK_NULL_HANDLE;
    }
}

ImageView::ImageView(ImageView&& other) noexcept
    : vulkanContext{ other.vulkanContext }
    , imageView{ other.imageView }
    , aspectMask{ other.aspectMask }
{
    other.vulkanContext = nullptr;
    other.imageView = VK_NULL_HANDLE;
    other.aspectMask = VK_IMAGE_ASPECT_NONE;
}

ImageView& ImageView::operator=(ImageView&& other) noexcept
{
    if (this != &other)
    {
        std::swap(vulkanContext, other.vulkanContext);
        std::swap(imageView, other.imageView);
        std::swap(aspectMask, other.aspectMask);
    }
    return *this;
}
