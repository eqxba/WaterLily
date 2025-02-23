#pragma once

#include "Utils/DataStructures.hpp"
#include "Engine/Render/Resources/ResourceHelpers.hpp"

#include <volk.h>

class VulkanContext;
class Image;
struct ImageDescription;

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

    VkImageView GetVkImageView() const
    {
        return imageView;
    }

private:    
    const VulkanContext* vulkanContext = nullptr;

    VkImageView imageView = VK_NULL_HANDLE;
};
