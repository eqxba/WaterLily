#define VMA_IMPLEMENTATION

#include "Engine/Render/Vulkan/Managers/MemoryManager.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"

namespace MemoryManagerDetails
{
    template<typename T>
    static VmaAllocation GetAllocation(const T resource, std::unordered_map<T, VmaAllocation>& allocations)
    {
        const auto it = allocations.find(resource);
        Assert(it != allocations.end());

        return it->second;
    }
}

MemoryManager::MemoryManager(const VulkanContext& aVulkanContext)
    : vulkanContext{aVulkanContext}
{   
    VmaVulkanFunctions vulkanFunctions{};
    vulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
    vulkanFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

    VmaAllocatorCreateInfo allocatorInfo{};
    allocatorInfo.vulkanApiVersion = VulkanConfig::apiVersion;
    allocatorInfo.physicalDevice = vulkanContext.GetDevice().GetPhysicalDevice();
    allocatorInfo.device = vulkanContext.GetDevice();
    allocatorInfo.pVulkanFunctions = &vulkanFunctions;
    allocatorInfo.instance = vulkanContext.GetInstance();

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

void MemoryManager::DestroyBuffer(const VkBuffer buffer)
{
    const VmaAllocation allocation = MemoryManagerDetails::GetAllocation(buffer, bufferAllocations);

    vmaDestroyBuffer(allocator, buffer, allocation);

    bufferAllocations.erase(buffer);
}

void MemoryManager::CopyMemoryToBuffer(const VkBuffer buffer, const std::span<const std::byte> data, const size_t offset)
{
    const VmaAllocation allocation = MemoryManagerDetails::GetAllocation(buffer, bufferAllocations);

    const VkResult result = vmaCopyMemoryToAllocation(allocator, data.data(), allocation, offset, data.size());
    Assert(result == VK_SUCCESS);
}

void* MemoryManager::MapBufferMemory(const VkBuffer buffer)
{
    const VmaAllocation allocation = MemoryManagerDetails::GetAllocation(buffer, bufferAllocations);

    void* mappedData;
    const VkResult result = vmaMapMemory(allocator, allocation, &mappedData);
    Assert(result == VK_SUCCESS);
    
    return mappedData;
}

void MemoryManager::UnmapBufferMemory(const VkBuffer buffer)
{
    const VmaAllocation allocation = MemoryManagerDetails::GetAllocation(buffer, bufferAllocations);

    vmaUnmapMemory(allocator, allocation);
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

void MemoryManager::DestroyImage(const VkImage image)
{
    const VmaAllocation allocation = MemoryManagerDetails::GetAllocation(image, imageAllocations);

    vmaDestroyImage(allocator, image, allocation);

    imageAllocations.erase(image);
}
