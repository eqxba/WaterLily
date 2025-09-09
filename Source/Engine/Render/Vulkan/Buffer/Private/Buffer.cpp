#include "Engine/Render/Vulkan/Buffer/Buffer.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"

namespace BufferDetails
{
    static VkBuffer CreateBuffer(const BufferDescription& description, const VulkanContext& vulkanContext)
    {
        Assert(description.size != 0);

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = description.size;
        bufferInfo.usage = description.usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        bufferInfo.queueFamilyIndexCount = 1;
        bufferInfo.pQueueFamilyIndices = &vulkanContext.GetDevice().GetQueues().familyIndices.graphicsAndComputeFamily;

        return vulkanContext.GetMemoryManager().CreateBuffer(bufferInfo, description.memoryProperties);
    }
}

Buffer::Buffer(BufferDescription aDescription, const bool createStagingBuffer, const VulkanContext& aVulkanContext)
    : vulkanContext{ &aVulkanContext }
    , buffer{ BufferDetails::CreateBuffer(aDescription, aVulkanContext) }
    , description{ std::move(aDescription) }
{
    if (createStagingBuffer)
    {
        CreateStagingBuffer();
    }
}

Buffer::~Buffer()
{
    if (!IsValid())
    {
        return;
    }

    if (!mappedMemory.empty())
    {
        vulkanContext->GetMemoryManager().UnmapBufferMemory(buffer);
        mappedMemory = {};
    }

    vulkanContext->GetMemoryManager().DestroyBuffer(buffer);
    buffer = VK_NULL_HANDLE;
}

Buffer::Buffer(Buffer&& other) noexcept
    : vulkanContext{ other.vulkanContext }
    , buffer{ other.buffer }
    , description{ other.description }
    , mappedMemory{ other.mappedMemory }
    , stagingBuffer{ std::move(other.stagingBuffer) }
{
    other.vulkanContext = nullptr;
    other.description = {};
    other.buffer = VK_NULL_HANDLE;
    other.mappedMemory = {};
    other.stagingBuffer = {};
}

Buffer& Buffer::operator=(Buffer&& other) noexcept
{
    if (this != &other)
    {
        std::swap(vulkanContext, other.vulkanContext);
        std::swap(buffer, other.buffer);
        std::swap(description, other.description);
        std::swap(mappedMemory, other.mappedMemory);
        std::swap(stagingBuffer, other.stagingBuffer);
    }

    return *this;
}

Buffer& Buffer::CreateStagingBuffer()
{
    Assert(!stagingBuffer);

    BufferDescription stagingBufferDescription = {
        .size = description.size,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT };

    stagingBuffer = std::make_unique<Buffer>(stagingBufferDescription, false, *vulkanContext);

    return *stagingBuffer;
}

void Buffer::DestroyStagingBuffer()
{
    Assert(stagingBuffer);

    stagingBuffer.reset();
}

std::span<std::byte> Buffer::MapMemory()
{
    Assert((description.memoryProperties & (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
        == (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));

    if (mappedMemory.empty())
    {
        void* mappedMemoryPointer = vulkanContext->GetMemoryManager().MapBufferMemory(buffer);
        mappedMemory = { static_cast<std::byte*>(mappedMemoryPointer), description.size };
    }

    return mappedMemory;
}

void Buffer::UnmapMemory()
{
    Assert(!mappedMemory.empty());

    vulkanContext->GetMemoryManager().UnmapBufferMemory(buffer);
    mappedMemory = {};
}

void Buffer::FillImpl(const std::span<const std::byte> data, const size_t offset /* = 0 */)
{
    Assert(!data.empty() && (data.size() + offset <= description.size));

    vulkanContext->GetMemoryManager().CopyMemoryToBuffer(buffer, data, offset);
}
