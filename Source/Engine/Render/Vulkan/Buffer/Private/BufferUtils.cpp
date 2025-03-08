#include "Engine/Render/Vulkan/Buffer/BufferUtils.hpp"

void BufferUtils::CopyBufferToBuffer(const VkCommandBuffer commandBuffer, const Buffer& source, 
    const Buffer& destination, size_t size /* = std::numeric_limits<size_t>::max() */,
    const size_t sourceOffset /* = 0 */, const size_t destinationOffset /* = 0 */)
{
    Assert(size != 0);

    if (size == std::numeric_limits<size_t>::max())
    {
        size = source.GetDescription().size;
    }

    Assert(size + sourceOffset <= source.GetDescription().size);
    Assert(size + destinationOffset <= destination.GetDescription().size);

    const VkBufferCopy copyRegion = { .srcOffset = sourceOffset, .dstOffset = destinationOffset, .size = size };
    vkCmdCopyBuffer(commandBuffer, source, destination, 1, &copyRegion);
}