#pragma once

#include "Engine/Render/Vulkan/Buffer/Buffer.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Synchronization/SynchronizationUtils.hpp"

namespace BufferUtils
{
    void CopyBufferToBuffer(VkCommandBuffer commandBuffer, const Buffer& source, const Buffer& destination, 
        size_t size = std::numeric_limits<size_t>::max(), size_t sourceOffset = 0, size_t destinationOffset = 0);

    template <typename... Buffers> requires (std::same_as<Buffers, Buffer> && ...)
    void UploadFromStagingBuffers(const VulkanContext& vulkanContext, const PipelineBarrier barrier,
        const bool destroyStagingBuffers, Buffers&... buffers)
    {
        vulkanContext.GetDevice().ExecuteOneTimeCommandBuffer([&](VkCommandBuffer cmd) {
            (CopyBufferToBuffer(cmd, buffers.GetStagingBuffer(), buffers), ...);
            SynchronizationUtils::SetMemoryBarrier(cmd, barrier);
        });

        if (destroyStagingBuffers)
        {
            (buffers.DestroyStagingBuffer(), ...);
        }
    }
}
