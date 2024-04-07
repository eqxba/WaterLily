#pragma once

#include "Utils/DataStructures.hpp"

#include <volk.h>

class VulkanContext;

struct ImageDescription
{
    std::string filePath;
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

    VkImageView GetImageView() const
    {
        return imageView;
    }

    VkSampler GetSampler() const
    {
        return sampler;
    }

private:
    const VulkanContext* vulkanContext = nullptr;

    ImageDescription description = {};

    VkImage image = VK_NULL_HANDLE;
    
    VkImageView imageView = VK_NULL_HANDLE; // TODO: do we have to store this here?
    VkSampler sampler = VK_NULL_HANDLE; // TODO: move away to a proper location, samplers can be used with different images
    // Think of a manager which cares about image-sampler compatibility? 

    Extent2D extent = {};
};
