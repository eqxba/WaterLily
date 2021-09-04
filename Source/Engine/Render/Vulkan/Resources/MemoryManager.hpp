#pragma once

#include <volk.h>

#pragma warning(push, 0)
#include <vk_mem_alloc.h>
#pragma warning(pop)

class MemoryManager
{
public:
    MemoryManager();
    ~MemoryManager();

    VkBuffer CreateBuffer(const VkBufferCreateInfo& bufferCreateInfo, const VkMemoryPropertyFlags memoryProperties);
    void DestroyBuffer(VkBuffer buffer);

private:    
    VmaAllocator allocator;

    std::unordered_map<VkBuffer, VmaAllocation> bufferAllocations;
};