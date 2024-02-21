#include "Engine/Render/Vulkan/Resources/BufferHelpers.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"

VkBuffer BufferHelpers::CreateStagingBuffer(const VulkanContext& vulkanContext, VkDeviceSize size)
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferInfo.queueFamilyIndexCount = 1;
    bufferInfo.pQueueFamilyIndices = &vulkanContext.GetDevice().GetQueues().familyIndices.graphicsFamily;

    const VkMemoryPropertyFlags memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT  | 
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    return vulkanContext.GetMemoryManager().CreateBuffer(bufferInfo, memoryProperties);
}