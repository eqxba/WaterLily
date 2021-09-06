#pragma once

#include "Engine/Render/Vulkan/Resources/BufferHelpers.hpp"

#include <volk.h>

// TODO: Think about API clarity of this class
class BufferManger
{
public:
    ~BufferManger();

    VkBuffer CreateBuffer(const BufferDescription& bufferDescription, bool createStagingBuffer = true);
    void DestroyBuffer(VkBuffer buffer);

    void FillBuffer(VkBuffer buffer, void* data, size_t size);
    void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

private:
    struct BufferEntry
    {
        BufferDescription description;
        VkBuffer stagingBuffer;
    };

    std::unordered_map<VkBuffer, BufferEntry> buffers;
};