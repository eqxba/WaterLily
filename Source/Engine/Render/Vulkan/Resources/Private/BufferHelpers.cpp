#include "Engine/Render/Vulkan/Resources/BufferHelpers.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"

VkBuffer BufferHelpers::CreateStagingBuffer(VkDeviceSize size)
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferInfo.queueFamilyIndexCount = 1;
    bufferInfo.pQueueFamilyIndices = &VulkanContext::device->queues.familyIndices.graphicsFamily;

    const VkMemoryPropertyFlags memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT  | 
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    return VulkanContext::memoryManager->CreateBuffer(bufferInfo, memoryProperties);
}