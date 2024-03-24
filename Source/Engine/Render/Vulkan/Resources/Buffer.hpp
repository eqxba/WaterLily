#pragma once

#include "Engine/Render/Vulkan/Resources/BufferHelpers.hpp"

#include <volk.h>

class VulkanContext;

class Buffer
{
public:
    Buffer(BufferDescription description, const VulkanContext& vulkanContext);

    template <typename T>
    Buffer(BufferDescription description, const VulkanContext& vulkanContext, const std::span<const T> initialData);

    ~Buffer();

    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    Buffer(Buffer&& other) noexcept;
    Buffer& operator=(Buffer&& other) noexcept;

    template <typename T>
    void Fill(const std::span<const T> data);    

    VkBuffer GetVkBuffer() const
    {
        return buffer;
    }

private:
    void FillImpl(const std::span<const std::byte> span);

    const VulkanContext& vulkanContext;

    const BufferDescription description;

    VkBuffer buffer = VK_NULL_HANDLE;
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
};

template <typename T>
Buffer::Buffer(BufferDescription description, const VulkanContext& vulkanContext, const std::span<const T> initialData)
    : Buffer{ std::move(description), vulkanContext }
{
    if (!initialData.empty())
    {
        Fill(initialData);
    }
}

template <typename T>
void Buffer::Fill(const std::span<const T> span)
{
    FillImpl(std::as_bytes(span));
}