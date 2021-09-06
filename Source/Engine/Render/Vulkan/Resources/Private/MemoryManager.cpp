#define VMA_IMPLEMENTATION

#include "Engine/Render/Vulkan/Resources/MemoryManager.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"

MemoryManager::MemoryManager()
{
    VmaAllocatorCreateInfo allocatorInfo{};
    allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_2;
    allocatorInfo.physicalDevice = VulkanContext::device->physicalDevice;
    allocatorInfo.device = VulkanContext::device->device;
    allocatorInfo.instance = VulkanContext::instance->instance;

    vmaCreateAllocator(&allocatorInfo, &allocator);
}

MemoryManager::~MemoryManager()
{
    vmaDestroyAllocator(allocator);
}

VkBuffer MemoryManager::CreateBuffer(const VkBufferCreateInfo& bufferCreateInfo, 
    const VkMemoryPropertyFlags memoryProperties)
{
    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocInfo.requiredFlags = memoryProperties;

    VkBuffer buffer;
    VmaAllocation allocation;

    const VkResult result = vmaCreateBuffer(allocator, &bufferCreateInfo, &allocInfo, &buffer, &allocation, nullptr);
	Assert(result == VK_SUCCESS);

    bufferAllocations.emplace(buffer, allocation);

    return buffer;
}

void MemoryManager::DestroyBuffer(VkBuffer buffer)
{
    const auto it = bufferAllocations.find(buffer);
    Assert(it != bufferAllocations.end());

    vmaDestroyBuffer(allocator, buffer, it->second);

    bufferAllocations.erase(it);
}

// TODO: Make generic on the 1st need
MemoryBlock MemoryManager::GetBufferMemoryBlock(VkBuffer buffer)
{
    const auto it = bufferAllocations.find(buffer);
    Assert(it != bufferAllocations.end());

    VmaAllocationInfo allocationInfo;
    vmaGetAllocationInfo(allocator, it->second, &allocationInfo);

    return MemoryBlock{ allocationInfo.deviceMemory, allocationInfo.offset, allocationInfo.size };
}

void* MemoryManager::MapBufferMemory(VkBuffer buffer)
{
    const auto it = bufferAllocations.find(buffer);
    Assert(it != bufferAllocations.end());

    void* mappedData;
    const VkResult result = vmaMapMemory(allocator, it->second, &mappedData);
    Assert(result == VK_SUCCESS);
    

    return mappedData;
}

void MemoryManager::UnmapBufferMemory(VkBuffer buffer)
{
    const auto it = bufferAllocations.find(buffer);
    Assert(it != bufferAllocations.end());

    vmaUnmapMemory(allocator, it->second);
}