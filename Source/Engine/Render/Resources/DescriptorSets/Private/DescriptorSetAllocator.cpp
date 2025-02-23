#include "Engine/Render/Resources/DescriptorSets/DescriptorSetAllocator.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Resources/DescriptorSets/DescriptorSetPool.hpp"

DescriptorSetAllocator::DescriptorSetAllocator() = default;

DescriptorSetAllocator::DescriptorSetAllocator(const uint32_t aMaxSetsInPool, 
    const std::span<const DescriptorSetPool::SizeRatio> aPoolRatios, const VulkanContext& aVulkanContext)
    : vulkanContext{&aVulkanContext}
    , maxSetsInPool{aMaxSetsInPool}
    , poolRatios{ aPoolRatios.begin(), aPoolRatios.end() }
{}

DescriptorSetAllocator::DescriptorSetAllocator(DescriptorSetAllocator&& other) noexcept
    : vulkanContext{other.vulkanContext}
    , maxSetsInPool{other.maxSetsInPool}
    , poolRatios{std::move(other.poolRatios)}
    , currentPool{other.currentPool}
    , usedPools{std::move(other.usedPools)}
    , freePools{std::move(other.freePools)}
{
    other.vulkanContext = nullptr;
    other.currentPool = nullptr;
}

DescriptorSetAllocator::~DescriptorSetAllocator() = default;

DescriptorSetAllocator& DescriptorSetAllocator::operator=(DescriptorSetAllocator&& other) noexcept
{
    if (this != &other)
    {
        std::swap(vulkanContext, other.vulkanContext);
        std::swap(maxSetsInPool, other.maxSetsInPool);
        std::swap(poolRatios, other.poolRatios);
        std::swap(currentPool, other.currentPool);
        std::swap(usedPools, other.usedPools);
        std::swap(freePools, other.freePools);
    }
    return *this;
}

VkDescriptorSet DescriptorSetAllocator::Allocate(const VkDescriptorSetLayout layout)
{
    if (!currentPool)
    {
        SetNewCurrentPool();
    }

    auto [result, set] = currentPool->Allocate(layout);

    if (result == VK_SUCCESS)
    {
        return set;
    }

    // Only 2 cases when we can try to reallocate, assert here means some major issues
    Assert(result == VK_ERROR_FRAGMENTED_POOL || result == VK_ERROR_OUT_OF_POOL_MEMORY);

    SetNewCurrentPool();

    std::tie(result, set) = currentPool->Allocate(layout);
    Assert(result == VK_SUCCESS);

    return set;
}

void DescriptorSetAllocator::ResetPools()
{
    std::ranges::move(usedPools, std::back_inserter(freePools));
    usedPools.clear();

    std::ranges::for_each(freePools, [](const std::unique_ptr<DescriptorSetPool>& pool) { pool->Reset(); });

    currentPool = nullptr;
}

void DescriptorSetAllocator::SetNewCurrentPool()
{
    if (freePools.empty())
    {
        usedPools.emplace_back(std::make_unique<DescriptorSetPool>(maxSetsInPool, poolRatios, *vulkanContext));
    }
    else
    {
        usedPools.push_back(std::move(freePools.back()));
        freePools.pop_back();
    }

    currentPool = usedPools.back().get();
}