#pragma once

#include <volk.h>

class VulkanContext;
class Device;

struct BufferDescription
{
    VkDeviceSize size;
    VkBufferUsageFlags usage;
    VkMemoryPropertyFlags memoryProperties;
    bool createStagingBuffer = true;
};

namespace BufferHelpers
{
    VkBuffer CreateStagingBuffer(VkDeviceSize size, const VulkanContext& vulkanContext);
    void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, const Device& device);
}
