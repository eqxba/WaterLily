#include "Engine/Render/Vulkan/Resources/Buffer.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"

Buffer::Buffer(BufferDescription aDescription, const VulkanContext& aVulkanContext)
    : description { std::move(aDescription) }
    , vulkanContext{ aVulkanContext }
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = description.size;
    bufferInfo.usage = description.usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    buffer = vulkanContext.GetMemoryManager().CreateBuffer(bufferInfo, description.memoryProperties);

    if (description.createStagingBuffer)
    {
        stagingBuffer = BufferHelpers::CreateStagingBuffer(description.size, vulkanContext);
    }
}

Buffer::~Buffer()
{
    MemoryManager& memoryManager = vulkanContext.GetMemoryManager();

    if (buffer != VK_NULL_HANDLE)
    {
        memoryManager.DestroyBuffer(buffer);
    }

    if (stagingBuffer != VK_NULL_HANDLE)
    {
        memoryManager.DestroyBuffer(stagingBuffer);
    }
}

Buffer::Buffer(Buffer&& other) noexcept
    : vulkanContext{ other.vulkanContext }
    , description{ other.description }
    , buffer{ other.buffer }
    , stagingBuffer{ other.stagingBuffer }
{
    other.buffer = VK_NULL_HANDLE;
    other.stagingBuffer = VK_NULL_HANDLE;
}

Buffer& Buffer::operator=(Buffer&& other) noexcept
{
    if (this != &other)
    {
        buffer = other.buffer;
        stagingBuffer = other.stagingBuffer;

        other.buffer = VK_NULL_HANDLE;
        other.stagingBuffer = VK_NULL_HANDLE;
    }
    return *this;
}

// TODO: 100% will need sync on this later
void Buffer::FillImpl(const std::span<const std::byte> span)
{
    MemoryManager& memoryManager = vulkanContext.GetMemoryManager();

    if ((description.memoryProperties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    {
        void* bufferMemory = memoryManager.MapBufferMemory(buffer);
        memcpy(bufferMemory, span.data(), span.size());
        memoryManager.UnmapBufferMemory(buffer);

        if ((description.memoryProperties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
        {
            const MemoryBlock memoryBlock = memoryManager.GetBufferMemoryBlock(buffer);

            VkMappedMemoryRange memoryRange{};
            memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
            memoryRange.memory = memoryBlock.memory;
            memoryRange.offset = memoryBlock.offset;
            memoryRange.size = memoryBlock.size;

            const VkResult result = vkFlushMappedMemoryRanges(vulkanContext.GetDevice().GetVkDevice(), 1, &memoryRange);
            Assert(result == VK_SUCCESS);
        }
    }
    else
    {
        Assert(stagingBuffer != VK_NULL_HANDLE);
        Assert((description.usage & VK_BUFFER_USAGE_TRANSFER_DST_BIT) == VK_BUFFER_USAGE_TRANSFER_DST_BIT);

        void* stagingBufferMemory = memoryManager.MapBufferMemory(stagingBuffer);
        memcpy(stagingBufferMemory, span.data(), span.size());
        memoryManager.UnmapBufferMemory(stagingBuffer);

        BufferHelpers::CopyBuffer(stagingBuffer, buffer, span.size(), vulkanContext.GetDevice());
    }
}