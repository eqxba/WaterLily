#pragma once

#include <volk.h>

#include "Engine/Render/Vulkan/VulkanConfig.hpp"

class VulkanContext;

class DescriptorPool
{
public:
    using SizeRatio = std::pair<VkDescriptorType, float>;

    DescriptorPool(const VulkanContext& vulkanContext, uint32_t maxSets, std::span<const SizeRatio> poolRatios);
    ~DescriptorPool();

    DescriptorPool(const DescriptorPool&) = delete;
    DescriptorPool& operator=(const DescriptorPool&) = delete;

    DescriptorPool(DescriptorPool&& other) noexcept;
    DescriptorPool& operator=(DescriptorPool&& other) noexcept;

    std::tuple<VkResult, VkDescriptorSet> Allocate(VkDescriptorSetLayout layout) const;

    void Reset() const;

private:
    const VulkanContext* vulkanContext = nullptr;

    VkDescriptorPool pool;
};

class DescriptorAllocator
{
public:
    using SizeRatio = std::pair<VkDescriptorType, float>;

    DescriptorAllocator();
	DescriptorAllocator(const VulkanContext& vulkanContext, uint32_t maxSetsInPool, 
        std::span<const SizeRatio> poolRatios);
	~DescriptorAllocator();

	DescriptorAllocator(const DescriptorAllocator&) = delete;
	DescriptorAllocator& operator=(const DescriptorAllocator&) = delete;

	DescriptorAllocator(DescriptorAllocator&& other) noexcept;
	DescriptorAllocator& operator=(DescriptorAllocator&& other) noexcept;

	VkDescriptorSet Allocate(VkDescriptorSetLayout layout);

	void ResetPools();

private:
    void SetNewCurrentPool();

	const VulkanContext* vulkanContext = nullptr;

    uint32_t maxSetsInPool = VulkanConfig::maxSetsInPool;
    std::vector<SizeRatio> poolRatios;

    DescriptorPool* currentPool = nullptr;

	std::vector<std::unique_ptr<DescriptorPool>> usedPools;
	std::vector<std::unique_ptr<DescriptorPool>> freePools;
};