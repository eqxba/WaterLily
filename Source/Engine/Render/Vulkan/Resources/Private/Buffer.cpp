#include "Engine/Render/Vulkan/Resources/Buffer.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"

namespace BufferDetails
{
    static VkBuffer CreateBuffer(BufferDescription description, const VulkanContext& vulkanContext)
    {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = description.size;
        bufferInfo.usage = description.usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        bufferInfo.queueFamilyIndexCount = 1;
        bufferInfo.pQueueFamilyIndices = &vulkanContext.GetDevice().GetQueues().familyIndices.graphicsFamily;

        return vulkanContext.GetMemoryManager().CreateBuffer(bufferInfo, description.memoryProperties);
    }

    // TODO: 100% will need sync on this later
    static void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, const Device& device)
    {
        device.ExecuteOneTimeCommandBuffer([&](VkCommandBuffer commandBuffer) {
            VkBufferCopy copyRegion{};
            copyRegion.srcOffset = 0; // Optional
            copyRegion.dstOffset = 0; // Optional
            copyRegion.size = size;
            vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
        });
    }
}

Buffer::Buffer(BufferDescription aDescription, bool createStagingBuffer, const VulkanContext* aVulkanContext)
    : vulkanContext{ aVulkanContext }
    , description { std::move(aDescription) }
{
    buffer = BufferDetails::CreateBuffer(description, *vulkanContext);

    if (createStagingBuffer)
    {
        BufferDescription stagingBufferDescription = { .size = description.size, 
            .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT };

        stagingBuffer = std::make_unique<Buffer>(stagingBufferDescription, false, vulkanContext);
    }   
}

Buffer::~Buffer()
{    
    if (!mappedMemory.empty())
    {
        Assert(persistentMapping);
        vulkanContext->GetMemoryManager().UnmapBufferMemory(buffer);
        mappedMemory = {};
    }

    if (buffer != VK_NULL_HANDLE)
    {
        vulkanContext->GetMemoryManager().DestroyBuffer(buffer);
        buffer = VK_NULL_HANDLE;
    }
}

Buffer::Buffer(Buffer&& other) noexcept
    : vulkanContext{ other.vulkanContext }
    , description{ other.description }
    , buffer{ other.buffer }
    , stagingBuffer{ std::move(other.stagingBuffer) }
    , mappedMemory{ other.mappedMemory }
    , persistentMapping{ other.persistentMapping }
{
    other.vulkanContext = nullptr;
    other.description = {};
    other.buffer = VK_NULL_HANDLE;
    other.stagingBuffer = {};
    other.mappedMemory = {};
    other.persistentMapping = false;
}

Buffer& Buffer::operator=(Buffer&& other) noexcept
{
    if (this != &other)
    {
        if (!mappedMemory.empty())
        {
            vulkanContext->GetMemoryManager().UnmapBufferMemory(buffer);
        }

        if (buffer != VK_NULL_HANDLE)
        {
            vulkanContext->GetMemoryManager().DestroyBuffer(buffer);
        }

        vulkanContext = other.vulkanContext;
        description = other.description;
        buffer = other.buffer;
        stagingBuffer = std::move(other.stagingBuffer);
        mappedMemory = other.mappedMemory;
        persistentMapping = other.persistentMapping;

        other.vulkanContext = nullptr;
        other.description = {};
        other.buffer = VK_NULL_HANDLE;
        other.stagingBuffer = {};
        other.mappedMemory = {};
        other.persistentMapping = false;
    }
    return *this;
}

std::span<std::byte> Buffer::MapMemory(bool aPersistentMapping /* = false */) const
{
    Assert((description.memoryProperties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    if (!persistentMapping)
    {
        Assert(mappedMemory.empty());
        persistentMapping = aPersistentMapping;
        void* mappedMemoryPointer = vulkanContext->GetMemoryManager().MapBufferMemory(buffer);
        mappedMemory = { static_cast<std::byte*>(mappedMemoryPointer), description.size };
    }

    return mappedMemory;
}

void Buffer::UnmapMemory() const
{
    if (!persistentMapping)
    {
        Assert(!mappedMemory.empty());
        vulkanContext->GetMemoryManager().UnmapBufferMemory(buffer);
        mappedMemory = {};
    }    
}

// TODO: 100% will need sync on this later
void Buffer::FillImpl(const std::span<const std::byte> span)
{
    Assert(span.size() == description.size);

    MemoryManager& memoryManager = vulkanContext->GetMemoryManager();
    const Device& device = vulkanContext->GetDevice();

    if ((description.memoryProperties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    {
        std::span<std::byte> memory = MapMemory();
        std::ranges::copy(span, memory.begin());
        UnmapMemory();

        if ((description.memoryProperties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
        {
            const MemoryBlock memoryBlock = memoryManager.GetBufferMemoryBlock(buffer);

            VkMappedMemoryRange memoryRange{};
            memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
            memoryRange.memory = memoryBlock.memory;
            memoryRange.offset = memoryBlock.offset;
            memoryRange.size = memoryBlock.size;

            const VkResult result = vkFlushMappedMemoryRanges(device.GetVkDevice(), 1, &memoryRange);
            Assert(result == VK_SUCCESS);
        }
    }
    else
    {
        Assert(stagingBuffer);
        Assert((description.usage & VK_BUFFER_USAGE_TRANSFER_DST_BIT) == VK_BUFFER_USAGE_TRANSFER_DST_BIT);

        stagingBuffer->Fill(span);
        BufferDetails::CopyBuffer(stagingBuffer->GetVkBuffer(), buffer, span.size(), device);
    }
}
