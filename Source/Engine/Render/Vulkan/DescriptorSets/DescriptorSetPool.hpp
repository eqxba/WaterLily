#pragma once

#include <volk.h>

class VulkanContext;

class DescriptorSetPool
{
public:
    using SizeRatio = std::pair<VkDescriptorType, float>;

    DescriptorSetPool(uint32_t maxSets, std::span<const SizeRatio> poolRatios, const VulkanContext& vulkanContext);
    ~DescriptorSetPool();

    DescriptorSetPool(const DescriptorSetPool&) = delete;
    DescriptorSetPool& operator=(const DescriptorSetPool&) = delete;

    DescriptorSetPool(DescriptorSetPool&& other) noexcept;
    DescriptorSetPool& operator=(DescriptorSetPool&& other) noexcept;

    std::tuple<VkResult, VkDescriptorSet> Allocate(VkDescriptorSetLayout layout) const;

    void Reset() const;

private:
    const VulkanContext* vulkanContext = nullptr;

    VkDescriptorPool pool;
};