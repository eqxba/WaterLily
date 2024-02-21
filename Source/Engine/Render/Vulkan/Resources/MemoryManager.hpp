#pragma once

#include <volk.h>

#pragma warning(push, 0)
#include <vk_mem_alloc.h>
#pragma warning(pop)

class VulkanContext;

struct MemoryBlock
{
    VkDeviceMemory memory;
    VkDeviceSize offset;
    VkDeviceSize size;
};

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

    MemoryBlock GetBufferMemoryBlock(VkBuffer buffer);
    void* MapBufferMemory(VkBuffer buffer);
    void UnmapBufferMemory(VkBuffer buffer);

private:    
    const VulkanContext& vulkanContext;

    VmaAllocator allocator;

    std::unordered_map<VkBuffer, VmaAllocation> bufferAllocations;
};