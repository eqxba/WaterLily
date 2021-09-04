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
