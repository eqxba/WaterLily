#pragma once

#include "Utils/DataStructures.hpp"
#include "Engine/Render/Vulkan/Resources/ResourceHelpers.hpp"

#include <volk.h>

class VulkanContext;
class Buffer;

struct ImageDescription
{
    Extent2D extent = {};
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
    Image(ImageDescription description, const VulkanContext* vulkanContext);

    ~Image();

    Image(const Image&) = delete;
    Image& operator=(const Image&) = delete;

    Image(Image&& other) noexcept;
    Image& operator=(Image&& other) noexcept;

    void FillMipLevel0(const Buffer& buffer, bool generateOtherMipLevels = false) const;

    const ImageDescription& GetDescription() const
    {
        return description;
    }

    const VkImage GetVkImage() const
    {
        return image;
    }

private:
    void TransitionLayout(VkCommandBuffer commandBuffer, ImageLayoutTransition transition) const;
    void TransitionLayout(VkCommandBuffer commandBuffer, ImageLayoutTransition transition, uint32_t baseMipLevel, 
        uint32_t mipLevelsCount = 1) const;

    void GenerateMipLevelsFromLevel0(VkCommandBuffer commandBuffer) const;

    const VulkanContext* vulkanContext = nullptr;

    ImageDescription description = {};

    VkImage image = VK_NULL_HANDLE;
};
