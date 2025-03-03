#include "Engine/Render/Vulkan/Resources/DescriptorSets/DescriptorSetLayoutBuilder.hpp"

#include "Engine/Render/Vulkan/Resources/DescriptorSets/DescriptorSetLayoutCache.hpp"
#include "Engine/Render/Vulkan/VulkanUtils.hpp"

DescriptorSetLayoutBuilder::DescriptorSetLayoutBuilder(DescriptorSetLayoutCache& aCache)
    : cache(&aCache)
{}

DescriptorSetLayout DescriptorSetLayoutBuilder::Build()
{
    Assert(!invalid);
    invalid = true;

    return cache->GetLayout(*this);
}

DescriptorSetLayoutBuilder& DescriptorSetLayoutBuilder::AddBinding(const uint32_t aBinding, const VkDescriptorType descriptorType,
    const VkShaderStageFlags shaderStages)
{
    Assert(!invalid);

    std::ranges::for_each(bindings, [&](const auto& binding) {
        Assert(binding.binding != aBinding);
    });

    VkDescriptorSetLayoutBinding binding{};
    binding.binding = aBinding;
    binding.descriptorCount = 1;
    binding.descriptorType = descriptorType;
    binding.stageFlags = shaderStages;

    bindings.push_back(binding);

    std::ranges::sort(bindings, [](const auto& lhs, const auto& rhs) {
        return lhs.binding < rhs.binding;
    });

    return *this;
}

const VkDescriptorSetLayoutBinding& DescriptorSetLayoutBuilder::GetBinding(const uint32_t index) const
{
    return VulkanUtils::GetBinding(bindings, index);
}

size_t DescriptorSetLayoutBuilder::GetHash()
{
    std::ranges::sort(bindings, [](const auto& lhs, const auto& rhs) {
        return lhs.binding < rhs.binding;
    });

    std::size_t seed = bindings.size();

    std::ranges::for_each(bindings, [&seed](const auto& binding) {
        seed ^= std::hash<VkDescriptorSetLayoutBinding>{}(binding)+0x9e3779b9 + (seed << 6) + (seed >> 2);
    });

    return seed;
}

std::vector<VkDescriptorSetLayoutBinding> DescriptorSetLayoutBuilder::MoveBindings()
{
    invalid = true;
    return std::move(bindings);
}