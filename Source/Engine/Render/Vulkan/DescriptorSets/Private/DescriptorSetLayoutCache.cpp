#include "Engine/Render/Vulkan/DescriptorSets/DescriptorSetLayoutCache.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"

DescriptorSetLayoutCache::DescriptorSetLayoutCache(const VulkanContext& aVulkanContext) 
    : vulkanContext{&aVulkanContext} 
{}

DescriptorSetLayoutCache::~DescriptorSetLayoutCache()
{
    std::ranges::for_each(layoutCache, [&](const auto& entry) {
        const auto& [info, layout] = entry;
        vkDestroyDescriptorSetLayout(vulkanContext->GetDevice(), layout, nullptr);
    });
}

DescriptorSetLayout DescriptorSetLayoutCache::GetLayout(DescriptorSetLayoutBuilder& layoutBuilder)
{
    const LayoutHash hash = layoutBuilder.GetHash();

    if (const auto it = layoutCache.find(hash); it != layoutCache.end())
    {
        Assert(layoutBindings.contains(hash));

        return { it->second, layoutBindings.at(hash) };
    }

    layoutBindings.emplace(hash, layoutBuilder.MoveBindings());
    const std::vector<VkDescriptorSetLayoutBinding>& bindings = layoutBindings.at(hash);

    VkDescriptorSetLayoutCreateInfo layoutCreateInfo{};
    layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutCreateInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutCreateInfo.pBindings = bindings.data();

    VkDescriptorSetLayout layout;

    const VkResult result = vkCreateDescriptorSetLayout(vulkanContext->GetDevice(), &layoutCreateInfo, 
        nullptr, &layout);
    Assert(result == VK_SUCCESS);

    layoutCache.emplace(hash, layout);

    return { layout, bindings };
}
