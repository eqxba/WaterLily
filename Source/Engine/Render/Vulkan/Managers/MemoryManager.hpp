#pragma once

#include <volk.h>

DISABLE_WARNINGS_BEGIN
#include <vk_mem_alloc.h>
DISABLE_WARNINGS_END

class VulkanContext;

class MemoryManager
{
public:
    MemoryManager(const VulkanContext& vulkanContext);
    ~MemoryManager();

    MemoryManager(const MemoryManager&) = delete;
    MemoryManager& operator=(const MemoryManager&) = delete;

    MemoryManager(MemoryManager&&) = delete;
    MemoryManager& operator=(MemoryManager&&) = delete;

    VkBuffer CreateBuffer(const VkBufferCreateInfo& bufferCreateInfo, const VkMemoryPropertyFlags memoryProperties);
    void DestroyBuffer(VkBuffer buffer);

    void CopyMemoryToBuffer(VkBuffer buffer, std::span<const std::byte> data, size_t offset);

    void* MapBufferMemory(VkBuffer buffer);
    void UnmapBufferMemory(VkBuffer buffer);

    VkImage CreateImage(const VkImageCreateInfo& imageCreateInfo, const VkMemoryPropertyFlags memoryProperties);
    void DestroyImage(VkImage image);

private:    
    const VulkanContext& vulkanContext;

   VmaAllocator allocator;

   std::unordered_map<VkBuffer, VmaAllocation> bufferAllocations;
   std::unordered_map<VkImage, VmaAllocation> imageAllocations;
};
