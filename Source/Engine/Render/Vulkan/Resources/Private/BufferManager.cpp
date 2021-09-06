#include "Engine/Render/Vulkan/Resources/BufferManager.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"

VkBuffer BufferManger::CreateBuffer(const BufferDescription& bufferDescription, bool createStagingBuffer /* = true */)
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferDescription.size;
    bufferInfo.usage = bufferDescription.usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkBuffer buffer = VulkanContext::memoryManager->CreateBuffer(bufferInfo, bufferDescription.memoryProperties);

    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    if (createStagingBuffer)
    {
        stagingBuffer = BufferHelpers::CreateStagingBuffer(bufferDescription.size);
    }

    buffers.emplace(buffer, BufferEntry{ bufferDescription, stagingBuffer });

    return buffer;
}

void BufferManger::DestroyBuffer(VkBuffer buffer)
{
    const auto& [description, stagingBuffer] = buffers.at(buffer);

    if (stagingBuffer != VK_NULL_HANDLE)
    {
        VulkanContext::memoryManager->DestroyBuffer(stagingBuffer);
    }

    VulkanContext::memoryManager->DestroyBuffer(buffer);

    buffers.erase(buffers.find(buffer));
}

// TODO: 100% will need sync on this later
void BufferManger::FillBuffer(VkBuffer buffer, void* data, size_t size)
{
    const auto& [description, stagingBuffer] = buffers.at(buffer);

    if ((description.memoryProperties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    {
        void* bufferMemory = VulkanContext::memoryManager->MapBufferMemory(buffer);
        memcpy(bufferMemory, data, size);
        VulkanContext::memoryManager->UnmapBufferMemory(buffer);

        if ((description.memoryProperties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
        {
            const MemoryBlock memoryBlock = VulkanContext::memoryManager->GetBufferMemoryBlock(buffer);

            VkMappedMemoryRange memoryRange{};
            memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
            memoryRange.memory = memoryBlock.memory;
            memoryRange.offset = memoryBlock.offset;
            memoryRange.size = memoryBlock.size;

            const VkResult result = vkFlushMappedMemoryRanges(VulkanContext::device->device, 1, &memoryRange);
            Assert(result == VK_SUCCESS);
        }
    }
    else
    {
        Assert(stagingBuffer != VK_NULL_HANDLE);
        Assert((description.usage & VK_BUFFER_USAGE_TRANSFER_DST_BIT) == VK_BUFFER_USAGE_TRANSFER_DST_BIT);

        void* stagingBufferMemory = VulkanContext::memoryManager->MapBufferMemory(stagingBuffer);
        memcpy(stagingBufferMemory, data, size);
        VulkanContext::memoryManager->UnmapBufferMemory(stagingBuffer);

        CopyBuffer(stagingBuffer, buffer, size);
    }
}

// TODO: 100% will need sync on this later
void BufferManger::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
    VulkanContext::device->ExecuteOneTimeCommandBuffer([&](VkCommandBuffer commandBuffer)
    {
        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0; // Optional
        copyRegion.dstOffset = 0; // Optional
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
    });
}