#pragma once

#include <volk.h>

class VulkanContext;

struct BufferDescription
{
    VkDeviceSize size = 0;
    VkBufferUsageFlags usage = {};
    VkMemoryPropertyFlags memoryProperties = {};
};

class Buffer
{
public:
    Buffer() = default;
    Buffer(BufferDescription description, bool createStagingBuffer, const VulkanContext& vulkanContext);

    template <typename T, size_t Extent>
    Buffer(BufferDescription description, bool createStagingBuffer, std::span<const T, Extent> initialData,
        const VulkanContext& vulkanContext);

    ~Buffer();

    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    Buffer(Buffer&& other) noexcept;
    Buffer& operator=(Buffer&& other) noexcept;

    Buffer& CreateStagingBuffer();
    void DestroyStagingBuffer();

    template <typename T, size_t Extent>
    void Fill(std::span<const T, Extent> data, size_t offset = 0);

    std::span<std::byte> MapMemory();
    void UnmapMemory();
    
    const BufferDescription& GetDescription() const
    {
        return description;
    }

    bool HasStagingBuffer() const
    {
        return stagingBuffer != nullptr;
    }

    Buffer& GetStagingBuffer()
    {
        return *stagingBuffer;
    }
    
    bool IsValid() const
    {
        return buffer != VK_NULL_HANDLE;
    }
    
    operator VkBuffer() const
    {
        return buffer;
    }

private:
    void FillImpl(std::span<const std::byte> data, size_t offset = 0);

    const VulkanContext* vulkanContext = nullptr;

    VkBuffer buffer = VK_NULL_HANDLE;

    BufferDescription description = {};

    std::span<std::byte> mappedMemory;

    std::unique_ptr<Buffer> stagingBuffer;
};

template <typename T, size_t Extent>
Buffer::Buffer(BufferDescription description, const bool createStagingBuffer, const std::span<const T, Extent> initialData,
    const VulkanContext& vulkanContext)
    : Buffer{ std::move(description), createStagingBuffer, vulkanContext }
{
    Buffer& bufferToFill = createStagingBuffer ? *stagingBuffer : *this;
    bufferToFill.FillImpl(std::as_bytes(initialData));
}

template <typename T, size_t Extent>
void Buffer::Fill(const std::span<const T, Extent> data, const size_t offset /* = 0 */)
{
    FillImpl(std::as_bytes(data), offset);
}
