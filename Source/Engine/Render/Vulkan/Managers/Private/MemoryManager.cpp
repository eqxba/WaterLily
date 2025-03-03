#define VMA_IMPLEMENTATION

#include "Engine/Render/Vulkan/Managers/MemoryManager.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"

MemoryManager::MemoryManager(const VulkanContext& aVulkanContext)
    : vulkanContext{aVulkanContext}
{   
    VmaVulkanFunctions vulkanFunctions{};
    vulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
    vulkanFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

    VmaAllocatorCreateInfo allocatorInfo{};
    allocatorInfo.vulkanApiVersion = VulkanConfig::apiVersion;
    allocatorInfo.physicalDevice = vulkanContext.GetDevice().GetPhysicalDevice();
    allocatorInfo.device = vulkanContext.GetDevice().GetVkDevice();
    allocatorInfo.pVulkanFunctions = &vulkanFunctions;
    allocatorInfo.instance = vulkanContext.GetInstance().GetVkInstance();

    vmaCreateAllocator(&allocatorInfo, &allocator);
}

MemoryManager::~MemoryManager()
{
    Assert(bufferAllocations.empty());
    Assert(imageAllocations.empty());
    vmaDestroyAllocator(allocator);
}

VkBuffer MemoryManager::CreateBuffer(const VkBufferCreateInfo& bufferCreateInfo, 
    const VkMemoryPropertyFlags memoryProperties)
{
    VmaAllocationCreateInfo allocInfo{};
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

void MemoryManager::FlushBuffer(VkBuffer buffer)
{
    const auto it = bufferAllocations.find(buffer);
    Assert(it != bufferAllocations.end());
    
    vmaFlushAllocation(allocator, it->second, 0, VK_WHOLE_SIZE);
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

VkImage MemoryManager::CreateImage(const VkImageCreateInfo& imageCreateInfo, 
    const VkMemoryPropertyFlags memoryProperties)
{
    VmaAllocationCreateInfo allocInfo{};
    allocInfo.requiredFlags = memoryProperties;

    VkImage image;
    VmaAllocation allocation;

    const VkResult result = vmaCreateImage(allocator, &imageCreateInfo, &allocInfo, &image, &allocation, nullptr);
    Assert(result == VK_SUCCESS);

    imageAllocations.emplace(image, allocation);

    return image;
}

void MemoryManager::DestroyImage(VkImage image)
{
    const auto it = imageAllocations.find(image);
    Assert(it != imageAllocations.end());

    vmaDestroyImage(allocator, image, it->second);

    imageAllocations.erase(it);
}
