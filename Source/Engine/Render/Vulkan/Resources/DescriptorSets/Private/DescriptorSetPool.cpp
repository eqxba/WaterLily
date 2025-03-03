#include "Engine/Render/Vulkan/Resources/DescriptorSets/DescriptorSetPool.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"

DescriptorSetPool::DescriptorSetPool(const uint32_t maxSets, const std::span<const SizeRatio> poolRatios
    , const VulkanContext& aVulkanContext)
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

DescriptorSetPool::~DescriptorSetPool()
{
    if (pool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(vulkanContext->GetDevice().GetVkDevice(), pool, nullptr);
    }
}

DescriptorSetPool::DescriptorSetPool(DescriptorSetPool&& other) noexcept
    : vulkanContext{ other.vulkanContext }
    , pool{ other.pool}
{
    other.vulkanContext = nullptr;
    other.pool = VK_NULL_HANDLE;
}

DescriptorSetPool& DescriptorSetPool::operator=(DescriptorSetPool&& other) noexcept
{
    if (this != &other)
    {
        std::swap(vulkanContext, other.vulkanContext);
        std::swap(pool, other.pool);
    }

    return *this;
}

std::tuple<VkResult, VkDescriptorSet> DescriptorSetPool::Allocate(const VkDescriptorSetLayout layout) const
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

void DescriptorSetPool::Reset() const
{
    if (pool != VK_NULL_HANDLE)
    {
        vkResetDescriptorPool(vulkanContext->GetDevice().GetVkDevice(), pool, 0);
    }
}