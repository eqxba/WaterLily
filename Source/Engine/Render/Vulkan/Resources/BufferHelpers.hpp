#pragma once

#include <volk.h>

struct BufferDescription
{
    VkDeviceSize size;
    VkBufferUsageFlags usage;
    VkMemoryPropertyFlags memoryProperties;
};

namespace BufferHelpers
{
    VkBuffer CreateStagingBuffer(VkDeviceSize size);
}
