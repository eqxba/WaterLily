#pragma once

#include <volk.h>

class VulkanContext;

struct ImageDescription
{
    VkExtent3D extent = {};
    uint32_t mipLevelsCount = 1;
    VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
    VkFormat format = {};
    VkBufferUsageFlags usage = {};
    VkMemoryPropertyFlags memoryProperties = {};
};

class Image
{
public:
    Image() = default;
    Image(VkImage image, ImageDescription description, const VulkanContext& vulkanContext, bool isSwapchainImage = false);
    Image(ImageDescription description, const VulkanContext& vulkanContext);

    ~Image();

    Image(const Image&) = delete;
    Image& operator=(const Image&) = delete;

    Image(Image&& other) noexcept;
    Image& operator=(Image&& other) noexcept;
    
    const ImageDescription& GetDescription() const
    {
        return description;
    }
    
    bool IsValid() const
    {
        return image != VK_NULL_HANDLE;
    }
    
    operator VkImage() const
    {
        return image;
    }
    
private:
    const VulkanContext* vulkanContext = nullptr;

    VkImage image = VK_NULL_HANDLE;
    
    ImageDescription description = {};
    
    bool isSwapchainImage = false;
};
