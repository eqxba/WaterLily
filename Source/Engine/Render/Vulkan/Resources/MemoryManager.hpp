#pragma once

#include <volk.h>

#pragma warning(push, 0)
#include <vk_mem_alloc.h>
#pragma warning(pop)

struct MemoryBlock
{
    VkDeviceMemory memory;
    VkDeviceSize offset;
    VkDeviceSize size;
};

class MemoryManager
{
public:
    MemoryManager();
    ~MemoryManager();

    VkBuffer CreateBuffer(const VkBufferCreateInfo& bufferCreateInfo, const VkMemoryPropertyFlags memoryProperties);
    void DestroyBuffer(VkBuffer buffer);

    MemoryBlock GetBufferMemoryBlock(VkBuffer buffer);
    void* MapBufferMemory(VkBuffer buffer);
    void UnmapBufferMemory(VkBuffer buffer);

private:    
    VmaAllocator allocator;

    std::unordered_map<VkBuffer, VmaAllocation> bufferAllocations;
};