#pragma once

#include "Engine/Render/Vulkan/Resources/BufferHelpers.hpp"

#include <volk.h>

class VulkanContext;

// TODO: Think about API clarity of this class
class BufferManager
{
public:
    BufferManager(const VulkanContext& vulkanContext);
    ~BufferManager();

    BufferManager(const BufferManager&) = delete;
    BufferManager& operator=(const BufferManager&) = delete;

    BufferManager(BufferManager&&) = delete;
    BufferManager& operator=(BufferManager&&) = delete;

    VkBuffer CreateBuffer(const BufferDescription& bufferDescription, bool createStagingBuffer = true);
    void DestroyBuffer(VkBuffer buffer);

    // TODO: Use span!
    void FillBuffer(VkBuffer buffer, void* data, size_t size);
    void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

private:
    struct BufferEntry
    {
        BufferDescription description;
        VkBuffer stagingBuffer;
    };

    const VulkanContext& vulkanContext;

    std::unordered_map<VkBuffer, BufferEntry> buffers;
};