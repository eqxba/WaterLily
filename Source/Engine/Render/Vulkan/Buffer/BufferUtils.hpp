#pragma once

#include "Engine/Render/Vulkan/Buffer/Buffer.hpp"

namespace BufferUtils
{
    void CopyBufferToBuffer(VkCommandBuffer commandBuffer, const Buffer& source, const Buffer& destination, 
        size_t size = std::numeric_limits<size_t>::max(), size_t sourceOffset = 0, size_t destinationOffset = 0);
}