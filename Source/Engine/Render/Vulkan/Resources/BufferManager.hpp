#pragma once

#include "Engine/Render/Vulkan/Resources/BufferHelpers.hpp"

#include <volk.h>

class BufferManger
{
public:
    VkBuffer CreateBuffer(const BufferDescription& bufferDescription, bool createStagingBuffer = true);
    void DestroyBuffer(VkBuffer buffer);

private:
    struct BufferEntry
    {
        BufferDescription description;
        VkBuffer stagingBuffer;
    };

    std::unordered_map<VkBuffer, BufferEntry> buffers;
};