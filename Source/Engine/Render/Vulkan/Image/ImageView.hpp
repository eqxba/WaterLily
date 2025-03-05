#pragma once

#include <volk.h>

#include "Engine/Render/Vulkan/Image/Image.hpp"

class VulkanContext;

class ImageView
{
public:
    ImageView() = default;   
    ImageView(VkImage image, const ImageDescription& description, VkImageAspectFlags aspectFlags, 
        const VulkanContext& vulkanContext);
    ImageView(const Image& image, VkImageAspectFlags aspectFlags, const VulkanContext& vulkanContext);

    ~ImageView();

    ImageView(const ImageView&) = delete;
    ImageView& operator=(const ImageView&) = delete;

    ImageView(ImageView&& other) noexcept;
    ImageView& operator=(ImageView&& other) noexcept;
    
    bool IsValid() const
    {
        return imageView != VK_NULL_HANDLE;
    }
    
    operator VkImageView() const
    {
        return imageView;
    }

private:    
    const VulkanContext* vulkanContext = nullptr;

    VkImageView imageView = VK_NULL_HANDLE;
};
