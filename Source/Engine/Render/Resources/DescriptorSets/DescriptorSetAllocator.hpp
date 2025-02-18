#pragma once

#include <volk.h>

#include "Engine/Render/Vulkan/VulkanConfig.hpp"

class VulkanContext;
class DescriptorSetPool;

class DescriptorSetAllocator
{
public:
    using SizeRatio = std::pair<VkDescriptorType, float>;

    DescriptorSetAllocator();
    DescriptorSetAllocator(uint32_t maxSetsInPool, std::span<const SizeRatio> poolRatios, 
        const VulkanContext& vulkanContext);
    ~DescriptorSetAllocator();

    DescriptorSetAllocator(const DescriptorSetAllocator&) = delete;
    DescriptorSetAllocator& operator=(const DescriptorSetAllocator&) = delete;

    DescriptorSetAllocator(DescriptorSetAllocator&& other) noexcept;
    DescriptorSetAllocator& operator=(DescriptorSetAllocator&& other) noexcept;

    VkDescriptorSet Allocate(VkDescriptorSetLayout layout);

    void ResetPools();

private:
    void SetNewCurrentPool();

    const VulkanContext* vulkanContext = nullptr;

    uint32_t maxSetsInPool = VulkanConfig::maxSetsInPool;
    std::vector<SizeRatio> poolRatios;

    DescriptorSetPool* currentPool = nullptr;

    std::vector<std::unique_ptr<DescriptorSetPool>> usedPools;
    std::vector<std::unique_ptr<DescriptorSetPool>> freePools;
};