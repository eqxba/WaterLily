#pragma once

#include <volk.h>

class VulkanContext;

struct BufferDescription
{
    VkDeviceSize size;
    VkBufferUsageFlags usage;
    VkMemoryPropertyFlags memoryProperties;
};

namespace BufferHelpers
{
    VkBuffer CreateStagingBuffer(const VulkanContext& vulkanContext, VkDeviceSize size);
}
