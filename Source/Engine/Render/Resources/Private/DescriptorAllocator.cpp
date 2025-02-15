#include "Engine/Render/Resources/DescriptorAllocator.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"

DescriptorPool::DescriptorPool(const VulkanContext& aVulkanContext, const uint32_t maxSets,
    const std::span<const SizeRatio> poolRatios)
    : vulkanContext{ &aVulkanContext }
{
    std::vector<VkDescriptorPoolSize> poolSizes;

    std::ranges::transform(poolRatios, std::back_inserter(poolSizes), [&](const SizeRatio& ratio) {
        return VkDescriptorPoolSize{ .type = ratio.first,
            .descriptorCount = static_cast<uint32_t>(ratio.second * static_cast<float>(maxSets)) };
    });

    VkDescriptorPoolCreateInfo poolCreateInfo = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    poolCreateInfo.flags = 0;
    poolCreateInfo.maxSets = maxSets;
    poolCreateInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolCreateInfo.pPoolSizes = poolSizes.data();

    vkCreateDescriptorPool(vulkanContext->GetDevice().GetVkDevice(), &poolCreateInfo, nullptr, &pool);
}

DescriptorPool::~DescriptorPool()
{
    if (pool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(vulkanContext->GetDevice().GetVkDevice(), pool, nullptr);
    }
}

DescriptorPool::DescriptorPool(DescriptorPool&& other) noexcept
    : vulkanContext{ other.vulkanContext }
    , pool{ other.pool}
{
    other.vulkanContext = nullptr;
    other.pool = VK_NULL_HANDLE;
}

DescriptorPool& DescriptorPool::operator=(DescriptorPool&& other) noexcept
{
    if (this != &other)
    {
        vulkanContext = other.vulkanContext;
        pool = other.pool;

        other.vulkanContext = nullptr;
        other.pool = VK_NULL_HANDLE;
    }

    return *this;
}

std::tuple<VkResult, VkDescriptorSet> DescriptorPool::Allocate(const VkDescriptorSetLayout layout) const
{
    const VkDevice device = vulkanContext->GetDevice().GetVkDevice();

    VkDescriptorSetAllocateInfo allocateInfo = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    allocateInfo.pNext = nullptr;
    allocateInfo.descriptorPool = pool;
    allocateInfo.descriptorSetCount = 1;
    allocateInfo.pSetLayouts = &layout;

    VkDescriptorSet descriptorSet;
    const VkResult result = vkAllocateDescriptorSets(device, &allocateInfo, &descriptorSet);

    return { result, descriptorSet };
}

void DescriptorPool::Reset() const
{
    if (pool != VK_NULL_HANDLE)
    {
        vkResetDescriptorPool(vulkanContext->GetDevice().GetVkDevice(), pool, 0);
    }
}

DescriptorAllocator::DescriptorAllocator() = default;

DescriptorAllocator::DescriptorAllocator(const VulkanContext& aVulkanContext, const uint32_t aMaxSetsInPool,
    const std::span<const DescriptorPool::SizeRatio> aPoolRatios)
    : vulkanContext{&aVulkanContext}
    , maxSetsInPool{aMaxSetsInPool}
    , poolRatios{ aPoolRatios.begin(), aPoolRatios.end() }
{}

DescriptorAllocator::~DescriptorAllocator() = default;

DescriptorAllocator::DescriptorAllocator(DescriptorAllocator&& other) noexcept
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

DescriptorAllocator& DescriptorAllocator::operator=(DescriptorAllocator&& other) noexcept
{
    if (this != &other)
    {
        vulkanContext = other.vulkanContext;
        maxSetsInPool = other.maxSetsInPool;
        poolRatios = std::move(other.poolRatios);
        currentPool = other.currentPool;
        usedPools = std::move(other.usedPools);
        freePools = std::move(other.freePools);

        other.vulkanContext = nullptr;
        other.currentPool = nullptr;
    }
    return *this;
}

VkDescriptorSet DescriptorAllocator::Allocate(const VkDescriptorSetLayout layout)
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

void DescriptorAllocator::ResetPools()
{
    std::ranges::move(usedPools, std::back_inserter(freePools));
    usedPools.clear();

    std::ranges::for_each(freePools, [](const std::unique_ptr<DescriptorPool>& pool) { pool->Reset(); });

    currentPool = nullptr;
}

void DescriptorAllocator::SetNewCurrentPool()
{
    if (freePools.empty())
    {
        usedPools.emplace_back(std::make_unique<DescriptorPool>(*vulkanContext, maxSetsInPool, poolRatios));
    }
    else
    {
        usedPools.push_back(std::move(freePools.back()));
        freePools.pop_back();
    }

    currentPool = usedPools.back().get();
}