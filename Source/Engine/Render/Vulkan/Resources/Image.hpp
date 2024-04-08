#pragma once

#include "Utils/DataStructures.hpp"

#include <volk.h>

class VulkanContext;
class Buffer;

struct ImageDescription
{
    Extent2D extent = {};
    VkFormat format = {};
    VkBufferUsageFlags usage = {};
    VkMemoryPropertyFlags memoryProperties = {};
};

class Image
{
public:
    Image() = default;
    Image(ImageDescription description, const VulkanContext* vulkanContext);

    ~Image();

    Image(const Image&) = delete;
    Image& operator=(const Image&) = delete;

    Image(Image&& other) noexcept;
    Image& operator=(Image&& other) noexcept;

    void Fill(const Buffer& buffer) const;

    void TransitionLayout(VkImageLayout oldLayout, VkImageLayout newLayout) const;

    VkImageView CreateImageView(VkImageAspectFlags aspectFlags) const;

    const ImageDescription& GetDescription() const
    {
        return description;
    }

private:
    const VulkanContext* vulkanContext = nullptr;

    ImageDescription description = {};

    VkImage image = VK_NULL_HANDLE;        
};
